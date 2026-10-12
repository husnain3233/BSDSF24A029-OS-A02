# OS Assignment 02 - Report

## Feature 2: ls-v1.1.0 (Long Listing)

### Q1. What is the difference between stat() and lstat()? When is lstat() more appropriate in ls?

Both system calls fill a `struct stat` with a file's metadata (type, permissions, owner, size, timestamps). The difference is how they treat symbolic links:

- `stat()` follows the symlink and returns information about the file the link points to (the target).
- `lstat()` does not follow it and returns information about the symlink itself.

`ls -l` should use `lstat()` because the long listing has to describe the entry that is actually in the directory. For a symlink, `lstat()` gives:
- file type `l` (so `S_ISLNK()` is true), whereas `stat()` would report the target's type, such as `-` or `d`;
- the link's own size, which is the length of the target path;
- the link's own permissions and timestamps.

This lets `ls -l` print `name -> target`. A second reason is that `stat()` fails on a dangling (broken) symlink because the target does not exist, while `lstat()` still works. In a recursive listing, `lstat()` also stops the program from following a link to a directory and looping forever. `stat()` is the right choice only when the target's information is wanted, for example `ls -L`.

### Q2. How can bitwise operators and macros extract file type and permissions from st_mode?

`st_mode` is an integer in which different bit groups mean different things:

- The high bits (mask `S_IFMT`, octal 0170000) hold the file type.
- The next 3 bits are the special bits: setuid (`S_ISUID`), setgid (`S_ISGID`) and sticky (`S_ISVTX`).
- The lowest 9 bits are the permissions for owner, group and others (`S_IRUSR`, `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, and so on).

The bitwise AND operator (`&`) works as a mask. It keeps the bits we ask about and clears all the others.

File type: first mask with `S_IFMT`, then compare:

    if ((st.st_mode & S_IFMT) == S_IFDIR)   /* it is a directory */

The `S_ISDIR(mode)` macro does exactly this. Masking first is necessary because the type is a multi-bit value. Testing `mode & S_IFDIR` alone would also be true for block devices, since their code shares a bit with the directory code.

Permissions: each permission is one bit, so a single AND tells us if it is set:

    if (st.st_mode & S_IRUSR)   /* owner has read permission */

If the result is non-zero, the bit is set. My `mode_to_string()` function repeats this check for all 9 permission bits to build a string such as `drwxr-xr-x`. For the execute positions it also checks `S_ISUID`, `S_ISGID` and `S_ISVTX` and shows `s`/`S` or `t`/`T` when the special bits are set.
