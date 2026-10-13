# OS Assignment 02 - Report

## Feature 2: ls-v1.1.0 (Long Listing)

### Q1. Difference between stat() and lstat()? When to use lstat() in ls?

Both give information about a file (size, owner, permissions, time).

- stat() follows a symbolic link and gives info about the file it points to.
- lstat() does not follow it. It gives info about the link itself.

ls -l should use lstat() because it must show the link itself (type "l" and "name -> target"), not the file it points to.

### Q2. How do we get file type and permissions from st_mode?

st_mode is one number that stores both the file type and the permissions as bits. We use the & operator to check one part at a time.

- File type: (st_mode & S_IFMT) == S_IFDIR checks for a directory. S_ISDIR(st_mode) does the same thing.
- Permission: if (st_mode & S_IRUSR) is true, the owner has read permission.

My mode_to_string() function checks all 9 permission bits like this and builds a string such as drwxr-xr-x.

## Feature 3: ls-v1.2.0 (Column Display)

### Q1. Logic of "down then across" printing? Why is one loop not enough?

First I store all names in an array and find the longest name. Then:
- column width = longest name + 2
- columns = terminal width / column width
- rows = total names / columns (rounded up)

I print row by row. The name at row r and column c is names[r + c * rows]. So the names go down each column first.

One simple loop only prints names in order, one after another. But the screen prints one line at a time, so one line needs names from different places in the array. That needs a loop inside a loop.

### Q2. Purpose of ioctl? Problem with fixed 80 columns?

ioctl with TIOCGWINSZ asks the terminal for its current width, so the output fits the screen.

With a fixed 80 columns, a wide terminal wastes space and a small terminal breaks the layout. It also does not change when the user resizes the window.

## Feature 4: ls-v1.3.0 (Horizontal Display -x)

### Q1. Which is more complex, vertical or horizontal printing? Which needs more pre-calculation?

The vertical ("down then across") one is more complex. It must first calculate the number of columns and rows, and then find each name with names[r + c * rows]. The horizontal one needs only the column width. It prints names one by one and starts a new line when the next name does not fit in the terminal width.

### Q2. How did you manage the display modes (-l, -x, default)?

I used one integer variable called mode with three values: MODE_DEFAULT, MODE_LONG and MODE_HORIZ. getopt() sets it in main. Then do_ls() checks it: if MODE_LONG it calls do_ls_long(), if MODE_HORIZ it calls print_horizontal(), otherwise it calls print_columns().
