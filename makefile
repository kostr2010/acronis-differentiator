CC = gcc
CFLAGS = -Wall -g

GPATH = ./diff
TPATH = ./tree
UPATH = ./utils

TREE     = ${TPATH}/tree.o
TREE_HDR = ${TPATH}/tree.h

DIFF     = ${GPATH}/diff.o
DIFF_HDR = ${GPATH}/diff.h

LOG     = ${UPATH}/log.o
LOG_HDR = ${UPATH}/log.h

OBJS = ${LOG}     ${DIFF}     ${TREE}
HDRS = ${LOG_HDR} ${DIFF_HDR} ${TREE_HDR} ${UPATH}/assertm.h

main: ${OBJS} ${HDRS}
	${CC} ${CFLAGS} -o main.out main.c ${OBJS} 

clean:
	rm -f ${OBJS}
	rm ./output/*
	rm ./log/*
	rm ./dump/*

all: main

	