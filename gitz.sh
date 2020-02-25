#!/bin/bash

git stash
git pull
make
chmod 777 sproxy
chmod 777 cproxy
