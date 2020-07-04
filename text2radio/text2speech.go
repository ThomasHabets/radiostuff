// Simple speech to text, writing oggs to stdout.
package main

// export GOOGLE_APPLICATION_CREDENTIALS=auth.json

import (
	"bytes"
	"context"
	"flag"
	"io"
	"os"

	"cloud.google.com/go/texttospeech/apiv1"
	log "github.com/sirupsen/logrus"
	texttospeechpb "google.golang.org/genproto/googleapis/cloud/texttospeech/v1"
)

var (
	lang = flag.String("language", "en-US", "Language")
)

func main() {
	flag.Parse()

	if flag.NArg() != 1 {
		log.Fatalf("Need to give the text")
	}

	ctx := context.Background()
	log.Infof("Connecting…")
	client, err := texttospeech.NewClient(ctx)
	if err != nil {
		log.Fatal(err)
	}
	defer client.Close()

	req := &texttospeechpb.SynthesizeSpeechRequest{
		Input: &texttospeechpb.SynthesisInput{
			InputSource: &texttospeechpb.SynthesisInput_Text{
				Text: flag.Arg(0),
			},
		},
		Voice: &texttospeechpb.VoiceSelectionParams{
			LanguageCode: *lang,
			// Name:
			// SsmlGender:
		},
		AudioConfig: &texttospeechpb.AudioConfig{
			AudioEncoding: texttospeechpb.AudioEncoding_OGG_OPUS,
		},
	}
	log.Infof("Text2Speeching…")
	resp, err := client.SynthesizeSpeech(ctx, req)
	if err != nil {
		log.Fatalf("API call: %v", err)
	}

	log.Infof("Writing %d bytes…", len(resp.AudioContent))
	if _, err := io.Copy(os.Stdout, bytes.NewBuffer(resp.AudioContent)); err != nil {
		log.Fatal(err)
	}
}
