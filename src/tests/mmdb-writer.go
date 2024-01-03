package main

import (
	"encoding/csv"
	"encoding/hex"
	"io"
	"log"
	"net"
	"os"
	"strconv"
	"strings"

	"github.com/maxmind/mmdbwriter"
	"github.com/maxmind/mmdbwriter/inserter"
	"github.com/maxmind/mmdbwriter/mmdbtype"
)

func makeRecord(key, mmdbType, value string) mmdbtype.DataType {
	var mmdbValue mmdbtype.DataType
	switch mmdbType {
	case "bool":
		v, _ := strconv.ParseBool(value)
		mmdbValue = mmdbtype.Bool(v)
	case "bytes":
		v, _ := hex.DecodeString(value)
		mmdbValue = mmdbtype.Bytes(v)
	case "double":
		v, _ := strconv.ParseFloat(value, 64)
		mmdbValue = mmdbtype.Float64(v)
	case "float":
		v, _ := strconv.ParseFloat(value, 32)
		mmdbValue = mmdbtype.Float32(v)
	case "int32":
		v, _ := strconv.ParseInt(value, 10, 32)
		mmdbValue = mmdbtype.Int32(v)
	case "uint16":
		v, _ := strconv.ParseInt(value, 10, 16)
		mmdbValue = mmdbtype.Uint16(v)
	case "uint32":
		v, _ := strconv.ParseInt(value, 10, 32)
		mmdbValue = mmdbtype.Uint32(v)
	case "uint64":
		v, _ := strconv.ParseInt(value, 10, 64)
		mmdbValue = mmdbtype.Uint64(v)
	case "string":
		mmdbValue = mmdbtype.String(value)
	case "map":
		keys := strings.Split(key, "/")
		key = keys[0]
		mmdbValue = mmdbtype.Map{
			mmdbtype.String(keys[1]): mmdbtype.Map{
				mmdbtype.String(keys[2]): mmdbtype.String(value),
			},
		}
	}
	record := mmdbtype.Map{}
	record[mmdbtype.String(key)] = mmdbValue

	return record
}

func main() {
	writer, err := mmdbwriter.New(
		mmdbwriter.Options{
			DatabaseType:            "libvmod-geoip2",
			Description:             map[string]string{"en": "libvmod-geoip2 test database"},
			IncludeReservedNetworks: true,
			Inserter:                inserter.TopLevelMergeWith,
		},
	)
	if err != nil {
		log.Fatal(err)
	}

	file, err := os.Open("test-data.csv")
	if err != nil {
		log.Fatal(err)
	}

	reader := csv.NewReader(file)

	for {
		row, err := reader.Read()
		if err == io.EOF {
			break
		}
		if err != nil {
			log.Fatal(err)
		}

		_, network, err := net.ParseCIDR(row[0])
		if err != nil {
			log.Fatal(err)
		}

		record := makeRecord(row[1], row[2], row[3])
		err = writer.Insert(network, record)
		if err != nil {
			log.Fatal(err)
		}
	}

	file, err = os.Create("libvmod-geoip2.mmdb")
	if err != nil {
		log.Fatal(err)
	}

	_, err = writer.WriteTo(file)
	if err != nil {
		log.Fatal(err)
	}
}
