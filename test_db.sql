create database if not exists `test_db`;
use `test_db`;
create table if not exists `test_table`(
    id int auto_increment primary key,
    name varchar(64) default 'nameless',
    num int default 0
);