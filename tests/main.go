package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
)

type Test struct {
	ExitCode int `json:"exit_code"`
}

const TEST_PATH string = "./tests"

func main() {
	dirs, err := os.ReadDir(TEST_PATH)
	if err != nil {
		fmt.Printf("Failed to read tests due to %v\n", err)
		os.Exit(1)
	}

	for _, dir := range dirs {
		if !dir.Type().IsDir() {
			continue
		}

		valid, err := runDirTest(dir)
		if err != nil {
			fmt.Printf("Error when processing directory: '%s'\n", dir.Name())
			os.Exit(1)
		}

		if !valid {
			fmt.Printf("Test in directory '%s' failed\n", dir.Name())
			os.Exit(1)
		}
	}

	fmt.Println("All tests passed!")
}

/*
* Attempts to run the tests within a directory
* If a directory does not have a valid test setup, will return error
* Returned bool indicates whether test passed or failed
 */
func runDirTest(dir os.DirEntry) (bool, error) {
	testPath := fmt.Sprintf("%s/%s/test.json", TEST_PATH, dir.Name())
	testData, err := os.ReadFile(testPath)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to read file: %s\n", testPath)
		return false, err
	}

	var testExpects Test
	err = json.Unmarshal(testData, &testExpects)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to decode test json data into struct: %v\n", err)
		return false, err
	}

	compileCommand := exec.Command("./build/Compiler")
	if err := compileCommand.Run(); err != nil {
		fmt.Printf("Failed to run compiler command, error: %v\n", err)
		return false, err
	}

	runCommand := exec.Command("./build/input")
	if err := runCommand.Run(); err != nil {
		exitError, ok := err.(*exec.ExitError)
		if !ok {
			return false, err
		}

		if exitError.ExitCode() != testExpects.ExitCode {
			fmt.Fprintf(os.Stderr, "Expected exit code: %d, got %d\n", testExpects.ExitCode, exitError.ExitCode())
			return false, err
		}
	}

	return true, nil
}
