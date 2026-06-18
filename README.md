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

In LZ77, it's actually possible and useful to have the length exceed the offset. This works because we match/copy byte-by-byte. For example, if we have offset 4 and length 10: for compressing, we just keep matching in the buffer even in overlapping into our lookahead buffer. And decoding writes out the buffer as it goes, so newly written bytes can be copied.

LZSS specifically uses a one-bit flag to encode whether an output "token" is a literal byte or a reference. To keep byte-addressed operations efficient, the flags are packed into 8 flags in one byte, then the next 8 tokens follow. This lets literal bytes be mixed freely with references without needing further encoding. It can't be the most efficient to use 1 bit for every single literal and reference, as compressed data I've tested appears to have sizeable runs of references or literals, but I will leave this for a fancier compressor.

### Circular buffer and lookahead window

Conceptually, dictionary coders operate on a flat array of data. There is a relatively small search window before the current position and a tiny lookahead buffer after. 

In practice, we only need to keep a circular buffer that is at least as large as the search buffer plus lookahead buffer, then index mod buffer size. Circular buffers are especially suited for FIFO streams, which our sliding windows are. Maintaining lookahead requires us to read ahead of the current position, while stale data in the buffer is overwritten as needed. Since we keep our own buffers, LZ77 is perfectly suited for streaming data while using quite limited memory.

### Search window

Searching the search window is by far the most computationally intensive step. The simple brute-force approach is to naively string match through the whole window every single time. I implemented this at first to test the algorithm as a whole. This works and gives optimal matches, taking O(s) time on average, but in the worst case it could take O(s*m) time, where s = search length and m = match length. That still isn't too bad for a small search window like 4 KB, but it can be made a lot more efficient.

The data structure that makes this operate is the familiar hash table (something sorely missing in the C standard library), but I took several tricks from other implementations to make this really efficient. The key is the first 3 bytes of the substring. The hashes of the keys could be computed as a rolling hash. The hash table values are positions, since offsets would be constantly increasing as we go along. 

Each bucket has a chain, where we insert new positions at the head of the chain.
Instead of pointers to the next in the chain, we simply index into a table `prev_pos[pos]`. Because we inserted new positions at the head, the chain is strictly decreasing. When we search the chain, for each position we compute the offset and search the longest match (some may have not been true matches due to hash collisions). 

We can stop searching when the position falls out of the search window or we hit a predefined max chain search length. This means we don't have to explicitly delete any position in the hash table; old positions in the chain are naturally overwritten when we go through mod buffer size.

My original idea was to delete old entries as they fall out of the search window, but this complicates the code, as I believe the only way to delete a position directly in the hash table (without searching the whole hash table) is by rehashing the key that just fell out to find the bucket. Maybe anything past the head in a doubly-linked chain with prev and next tables could handle quick deletion? It very much takes away from the cleanness of the hash table.

Other data structures like suffix trees (or KMP?) can work, but they don't have the same simplicity. With l = max chain search length, usually something small like 64, the worst case of the hash table is limited to O(l*m), which is much smaller than the search window size.

