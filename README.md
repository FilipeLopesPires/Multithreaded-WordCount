# Multithreaded-WordCount
A Parallel Word and Vowel Counter

## Description

The goal of this project is to provide a fast computation program that reads in succession several text files and prints a listing of the occurring frequency of word lengths and the number of vowels in each word for each of the supplied
texts.
The source code contains three different implementations: one single-threaded, one multi-threaded and one resorting to the Message Passing Interface (MPI).

Notes: the project's dimension had to be reduced due to Covid-19 related constraints.

The execution times for the multithreaded version using a regular pc are:

<img src="https://github.com/FilipePires98/Multithreaded-WordCount/blob/master/diagrams/tables/multithreaded-exectime-problem1.jpg" width="480px">

For the MPI version, the times are:

<img src="https://github.com/FilipePires98/Multithreaded-WordCount/blob/master/diagrams/tables/mpi-exectime-problem1.jpg" width="440px">

## Repository Structure:

/dataset - contains the text files to be processed

/diagrams - contains visual representations of the program for analysis purposes

/src - contains the source code of all versions of the program

## Authors

The authors of this repository are Filipe Pires and João Alegria, and the project was developed for the Large Scale Computation Course of the Master's degree in Informatics Engineering of the University of Aveiro.

For further information, please contact us at filipesnetopires@ua.pt or joao.p@ua.pt.
