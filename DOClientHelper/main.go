package main

import (
	"fmt"
	"os"

	"github.com/akamensky/argparse"
	"gopkg.in/toast.v1"
)

var DOApplicationID string = "Deep Ocean Client Helper"

func ShowNormalToastNotification(title string, message string) (err error) {
	notification := toast.Notification{
		AppID:   DOApplicationID,
		Title:   title,
		Message: message,
	}

	err = notification.Push()
	if err != nil {
		return err
	}
	return nil
}

func main() {
	// Create new parser object
	parser := argparse.NewParser("Deep Ocean Client Helper", "The very deep utility for Deep Ocean Service (currently serve for toast notification")
	// Create title flag
	title := parser.String("t", "title", &argparse.Options{Required: true, Help: "Title of toast notification"})
	content := parser.String("c", "content", &argparse.Options{Required: true, Help: "Content of toast notification"})
	// Parse input
	err := parser.Parse(os.Args)
	if err != nil {
		// In case of error print error and print usage
		// This can also be done by passing -h or --help flags
		fmt.Print(parser.Usage(err))
	}
	// Finally print the collected string
	// fmt.Println(*title)
	// fmt.Println(*content)
	ShowNormalToastNotification(*title, *content)
}
