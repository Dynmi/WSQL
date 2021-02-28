# WSQL

## Introduction 

WSQL is a complete single-user relational database management system. 

## Features

- Superior performance for storage
- Superior performance for query

## Documentation

- [WSQL Internal Manual Documentation](doc/internal.md)
- [WSQL User Toturial Documentation](doc/user.md)

## Try it

### Install

On Windows, install `MinGW` first. Then use 
```
mingw32-make clean
mingw32-make all
```

On Linux, use 
```
sudo make clean && sudo make all
```

### Run

On Windows, use `.\wsql.exe`.

On Linux, use `./wsql`.

## Next version
- Visualization of database / table / column with ML algorithm
- **Brand new** query  optimization strategy
- Transactions and locking