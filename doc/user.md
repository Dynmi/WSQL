# WSQL Toturial

## Supported Comparision Operations

<table>
	<tr>
	    <th>= </th>
	    <th> != </th>
	    <th> < </th>  
	    <th> > </th>  
	    <th> <= </th>  
	    <th> >= </th>  
	</tr >
</table>

## Supported Data Type

<table>
	<tr>
	    <th> TYPE </th>
	    <th> LENGTH(Byte) </th>
	</tr >
	<tr>
	    <th> int整型 </th>
	    <th> 4 </th>
	</tr >
	<tr>
	    <th> string字符串型 </th>
	    <th> 1～MAXLEN </th>
	</tr >
	<tr>
	    <th> float浮点型 </th>
	    <th> 4 </th>
	</tr >
</table>

## Installation (Only support Linux)

```
git clone git@github.com:Dynmi/WSQL.git
cd WSQL
sudo make clean && sudo make all
```

## Usage

Use ```./wsql``` to start WSQL.

Each sentence should be ended with ";".

### DML

#### `list databases`

This will list all databases.

#### `create database <database name>`

A new database will be created.
Example:
```
WSQL > create database dbtest;
dbtest
PATH: ./
------------SUCCESS-------------
WSQL > 
```

#### `drop database <database name>`

Given database will be deleted. 
Example:
```
WSQL > drop database dbtest;
------------SUCCESS-------------
WSQL > 
```

#### `use database <database name>`

This will enter an existing database.
Example:
```
WSQL > use db2;
WSQL@db2 > 
```

#### `list tables`

This will list all tables in a database.
Example:
```
WSQL@db2 > list tables;
#==================================#
|           Table                  |
+----------------------------------+
| tb1                              |
#==================================#
------------SUCCESS-------------
WSQL@db2 > 
```

#### `detail table <table name>`

This will show the details of given table.
Example:
```
WSQL@db2 > detail table tb1;

   tb1  
--------------------------------------
NAME       TYPE         LENGTH(Byte) 
--------------------------------------
age       INT     4 
name      STRING  10 
--------------------------------------

------------SUCCESS-------------

```

#### `create table <table name> ...`

A new empty table will be created in database. 
Example:
```
WSQL@db2 > create table tbtest (id INT, height FLOAT, name STRING[12]);

------------------------------------------
CREATING TABLE tbtest
------------------------------------------
./db2/tbtest.scm
./db2/tbtest.id.data
./db2/tbtest.id.index
./db2/tbtest.height.data
./db2/tbtest.height.index
./db2/tbtest.name.data
./db2/tbtest.name.index
------------SUCCESS-------------
WSQL@db2 > 
```

#### `drop table <table name>`

Given table will be deleted.
Example:
```
WSQL@db2 > drop table tbtest;

------------------------------------------
DROPING TABLE tbtest
------------------------------------------
------------SUCCESS-------------
WSQL@db2 > 

```

#### `rename table <table name> <table name>`

Example:
```
WSQL@db2 > list tables;
#==================================#
|           Table                  |
+----------------------------------+
| tb1                              |
#==================================#
------------SUCCESS-------------
WSQL@db2 > rename table tb1 tbxxx;
------------SUCCESS-------------
WSQL@db2 > list tables;
#==================================#
|           Table                  |
+----------------------------------+
| tbxxx                            |
#==================================#
------------SUCCESS-------------
WSQL@db2 > 
```

#### `clear table <table name>`

#### `alter table <table name> rename column <old column name> <new column name>`

#### `alter table <table name> drop column <column name>`

#### `alter table <table name> add column (<column name> <column type>)`

### DDL

#### `update <table name> (<column name 1>,<column name 2>,...>):(<new value 1>, <new value 2>,...) where <where-condition>`


#### `insert into <table name> (<column name 1>,<column name 2>,...):(<new value 1>,<new value 2>,...)`

#### ~~`insert into <table name> select ...`~~

#### `delete from <table name> where <where-condition>`

#### `select * from <table name>`
#### `select * from <table name> where <where-condition>`
#### `select <column name 1>,<column name 2>,... from <table name>`
#### `select <column name 1>,<column name 2>,... from <table name> where <where-condition>`


## Remarks
- Lastest updated on 5th,March,2021