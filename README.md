# LZSS

This program implements the LZSS compression algorithm, a minor variant of LZ77. 

## How it works

LZ77 is fundamentally a dictionary coder, meaning it compresses by replacing strings within the input data with references to strings in the dictionary. 

How it does this is remarkably straightforward. 
As we iterate byte-by-byte through our data, we maintain a fixed-sized "lookback" search window, say exactly the previous 32k bytes from our current position. 
If we find a match between a substring starting at our current position and a substring in our search window, the current substring is replaced in the output with a reference in the form of an offset-length pair: the offset specifies how many bytes to go back, and the length specifies how many bytes to copy from there. 
Anything not a match is copied verbatim to the output. 

Decoding is the same process but in reverse. Whenever we encounter a reference, we look in our search buffer and replace it with its substring.

That's it! Everything else is implementation details.

## Implementation details

Many little details go into the implementation to make it very efficient.

### Reference offset-length pair

To actually save space, we want our matches to be longer than the replacement reference. This implementation follows DEFLATE with 1-2 byte offsets and 1 byte lengths, for a total of 2-3 bytes. The offset is stored as a variable-width 15-bit integer, with the idea that good matches should be recent and have a small offset, thus only using 1 byte. Specifically, my implementation is values 0-127 are stored as 0xxxxxxx, and 128-32767 are stored as 1yyyyyyy xxxxxxxx. In practice, longer match lengths are rare, so only 0-255 is needed for lengths.
(DEFLATE actually cleverly stores offsets 1-32768 and lengths 3-258 to squeeze out a few extra values.)

In LZ77, it's actually possible and useful to have the length exceed the offset. This works because we match/copy byte-by-byte. During compression, our string search just keeps on matching in the buffer even if our substrings overlap. And decompression writes out the buffer as it goes, so we can copy bytes we just wrote in the buffer.

LZSS specifically uses a one-bit flag to encode whether an output "token" is a literal byte or a reference. To keep byte-addressed operations efficient, the flags are packed into 8 flags in one byte, then the next 8 tokens follow. This lets literal bytes be mixed freely with references without needing further encoding (however for text data it doesn't take advantage of the byte distribution as an entropy coding would). 

It can't be the most efficient to use 1 bit for every single literal and reference, as compressed text data I've looked at seems to be mostly short runs of references and literals, which could be run-length encoded. Additionally, offsets tend to be quite large and lengths tend to be quite short. DEFLATE uses Huffman coding after LZ77, which could take advantage of this imbalance in output values. Most of the compression in DEFLATE is from LZ77, but Huffman contributes a sizeable minority, maybe 10-30%. 

### Circular buffer and lookahead window

Conceptually, dictionary coders operate on a flat array of data. There is a relatively small search window before the current position and a tiny lookahead buffer after. 

In practice, we only need to keep a circular buffer that is at least as large as the search buffer plus lookahead buffer, then index mod buffer size. Circular buffers are especially suited for FIFO streams, which our sliding windows are. Maintaining lookahead requires us to read ahead of the current position, while stale data in the buffer is overwritten as needed. Since we keep our own buffers, LZ77 is perfectly suited for streaming data while using quite limited memory.

### Search window

Searching the search window is by far the most computationally intensive step. The simple brute-force approach is to naively string match through the whole window every single time. I implemented this at first to test the algorithm as a whole. This works and gives optimal matches, taking O(s) time on average, but in the worst case it could take O(s*m) time, where s = search length and m = match length. That still isn't too bad for a small search window like 4 KB, but it can be made a lot more efficient.

The data structure that makes this operate is the familiar hash table (something sorely missing in the C standard library), but I took several tricks from other implementations to make this really efficient. The key is the first 3 bytes of the substring. The hashes of the keys could be computed as a rolling hash. The hash table values are positions, since offsets would be constantly increasing as we go along. 

Each bucket has a chain, where we insert new positions at the head of the chain.
Instead of pointers to the next in the chain, we simply index into a table `prev_pos[pos]`. Because we inserted new positions at the head, the chain is strictly decreasing. When we search the chain, for each position we compute the offset and search the longest match (some may have not been true matches due to hash collisions). 

We can stop searching when the position falls out of the search window or we hit a predefined max chain search length. This means we don't have to explicitly delete any position in the hash table; old positions in the chain are naturally overwritten as we go through mod buffer size.

My original idea was to delete old entries as they fall out of the search window, but this complicates the code, as I believe the only way to delete a position directly in the hash table (without searching the whole hash table) is by rehashing the key that just fell out to find the bucket. Maybe anything past the head in a doubly-linked chain with prev and next tables could handle quick deletion? It very much takes away from the cleanness of the hash table.

Other data structures like suffix trees (or KMP?) can work, but they don't have the same simplicity. With l = max chain search length, usually something small like 64, the worst case of the hash table is limited to O(l*m), which is much smaller than the search window size.

### Aside: Multiplicative hash

The hash function uses the "multiplication method" described in CLRS 3ed, Section 11.3.2. For an $p$-bit hash, i.e. mod $m = 2^p$:

$$h(k) = \lfloor m \lbrace kA \rbrace \rfloor$$

where $A$ is an irrational number between $0$ and $1$.
(The notation $\lbrace kA \rbrace$ means the fractional part of $kA$.)

Knuth suggests $A = \phi^{-1} = (\sqrt 5 - 1) / 2 \approx 0.6180339887 \dots$.  (see below for discussion)

Let's say we're using unsigned 32-bit int words.
We approximate $A = 2654435769 / 2^{32}$. Then the low-order 32-bit word of $k \times 2654435769$ approximates the fractional part $\lbrace kA \rbrace$. The most significant bits are higher quality (use more of the key), so we take the $p$ most significant bits, which is done by shifting right by $32 - p$.

The hash function is implemented simply:

```c
uint32_t hash(uint32_t key)
{
    return ((uint32_t)2654435769 * key) >> (32 - p);
}
```

For safety, the constant is specified as unsigned 32-bit to prevent any weird integer promotions to signed or 64-bit. I had a bug with this earlier where my types somehow resulted in a negative hash. What width is an [integer constant](https://en.cppreference.com/c/language/integer_constant)? It could be an `int` or `long` or `long long` (C99), but those widths are implementation-defined and only required to be at least certain widths.[^1] Just thinking about it pisses me off (this will go in my long blog post about everything that annoys me about C).

The great thing about this hash is it's only two CPU instructions: a multiply and a shift, so extremely fast. But how good is it as a hash function? 

The idea of multiplying the integer key by an irrational number is that we get another irrational number, with a uniformly distributed fractional part. 
This is actually a result from surprisingly deep number theory ideas, namely [Weyl's equidistribution theorem](https://www2.math.upenn.edu/~gressman/analysis/09-equidistribution.html). That property is asymptotic, so says nothing about "small" $k$. 
[The golden ratio is "maximally irrational"](https://cstheory.stackexchange.com/a/6203), i.e. has all small terms in its continued fraction. Therefore $k \phi$ isn't close to an integer for small $k$, and we can expect not too small or large hashes for small $k$, which seems like a good property. 

None of this has any real impact on the compression with a decent hash function, as collisions aren't important at all compared to the dictionary size. The dictionary size directly determines whether you search a far-back match or not. Modern compressors can outperform DEFLATE by using MB or even GB size (7z) dictionaries and taking advantage of long-distance patterns. 

See [the answers on the SO question](https://stackoverflow.com/questions/11871245/knuth-multiplicative-hash) and Knuth TAOCP Vol. 3, Section 6.4 Hashing for more details. Wow, I did not expect to write this much about the hash function.

[^1]: Eric Postpischil pointed out on SO that if a hypothetical implementation had 64-bit `int`, the multiplication will be 64-bit anyway.
