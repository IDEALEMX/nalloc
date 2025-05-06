# nalloc
A custom heap allocator built on c,
probably wont work on a non UNIX os.

Nalloc uses sbrk to request new memory,
and header memory blocks as metadata.

Contains the:
nalloc() \n
nalloc_free() \n
functions, used pretty much exactly as their malloc equivalents.
