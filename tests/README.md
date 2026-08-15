# Testing

This folder contains all the end-to-end tests for the compiler.

The aim of these tests is to check that the compiler produces the expected
output for the provided programs, but also provide documentation on how to 
use different features from the language.

## Structure

The testing utility is a seperate program written in Golang - source can be 
found in `main.go` 

Go has been chosen for this due to its simplicity for writing small utilites
such as this.

Other than the go files in this folder, the rest of the files are all tests.

Each test has its own individually numbered folder (e.g. `00 - Basic`) and
inside those folders is a mgs source file, and a json file containing the 
expected outputs.

When running the testing utility, we will go through all test cases and ensure
that the compiled output matches the expected values from the JSON.

## Running the tests

These tests are designed to be run from the root of the project using:
```
make test
```

They can also be run by directly running the go file from the project root with:
```
go run tests/main.go
```
