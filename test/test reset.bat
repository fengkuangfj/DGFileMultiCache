@echo off

sc stop dgcoreservice
sc start dgcoreservice
sc stop dgfile
sc start dgfile
sc stop dgcoreservice
sc start dgcoreservice