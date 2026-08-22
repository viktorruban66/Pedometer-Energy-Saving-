// pedometer.go
package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"sort"
	"strconv"
	"time"
)

type Config struct {
	StrideCm float64 `json:"stride_cm"`
	WeightKg float64 `json:"weight_kg"`
	Goal     int     `json:"goal"`
}

type Entry struct {
	Date  string `json:"date"`
	Steps int    `json:"steps"`
}

var configFile = "pedometer_config.json"
var dataFile = "steps.json"

func loadConfig() Config {
	var cfg Config
	cfg.StrideCm = 78.0
	cfg.WeightKg = 70.0
	cfg.Goal = 10000
	data, err := os.ReadFile(configFile)
	if err != nil {
		return cfg
	}
	json.Unmarshal(data, &cfg)
	return cfg
}

func saveConfig(cfg Config) {
	data, _ := json.MarshalIndent(cfg, "", "  ")
	os.WriteFile(configFile, data, 0644)
}

func loadEntries() []Entry {
	var entries []Entry
	data, err := os.ReadFile(dataFile)
	if err != nil {
		return entries
	}
	json.Unmarshal(data, &entries)
	return entries
}

func saveEntries(entries []Entry) {
	data, _ := json.MarshalIndent(entries, "", "  ")
	os.WriteFile(dataFile, data, 0644)
}

func getEntry(entries []Entry, date string) *Entry {
	for i, e := range entries {
		if e.Date == date {
			return &entries[i]
		}
	}
	return nil
}

func addSteps(entries []Entry, steps int, dateStr string, cfg Config) []Entry {
	if dateStr == "" {
		dateStr = time.Now().Format("2006-01-02")
	}
	e := getEntry(entries, dateStr)
	if e != nil {
		e.Steps = steps
	} else {
		entries = append(entries, Entry{Date: dateStr, Steps: steps})
	}
	saveEntries(entries)
	dist := float64(steps) * cfg.StrideCm / 100000.0
	cal := float64(steps) * 0.04 * cfg.WeightKg / 70.0
	fmt.Printf("✅ Logged %d steps on %s – %.2f km, %.0f kcal\n", steps, dateStr, dist, cal)
	return entries
}

func stats(entries []Entry, cfg Config) {
	if len(entries) == 0 {
		fmt.Println("No entries.")
		return
	}
	totalSteps := 0
	for _, e := range entries {
		totalSteps += e.Steps
	}
	totalDist := float64(totalSteps) * cfg.StrideCm / 100000.0
	totalCal := float64(totalSteps) * 0.04 * cfg.WeightKg / 70.0
	avgSteps := totalSteps / len(entries)
	today := time.Now().Format("2006-01-02")
	todaySteps := 0
	if e := getEntry(entries, today); e != nil {
		todaySteps = e.Steps
	}
	pct := int(float64(todaySteps) / float64(cfg.Goal) * 100)
	if pct > 100 {
		pct = 100
	}
	fuelSaved := totalDist / 10.0
	co2Saved := fuelSaved * 2.31

	fmt.Printf("\n🚶 Pedometer (Energy Saving)\n")
	fmt.Printf("Today: %d steps\n", todaySteps)
	fmt.Printf("Goal: %d steps (%d%% complete)\n", cfg.Goal, pct)
	fmt.Printf("\n📊 Statistics\n")
	fmt.Printf("Total steps: %d\n", totalSteps)
	fmt.Printf("Total distance: %.2f km\n", totalDist)
	fmt.Printf("Total calories: %.0f kcal\n", totalCal)
	fmt.Printf("Average: %d steps/day\n", avgSteps)
	fmt.Printf("Energy saved: %.2f L of fuel\n", fuelSaved)
	fmt.Printf("CO₂ saved: %.2f kg\n", co2Saved)
}

func listEntries(entries []Entry, cfg Config) {
	if len(entries) == 0 {
		fmt.Println("No entries.")
		return
	}
	sort.Slice(entries, func(i, j int) bool {
		return entries[i].Date > entries[j].Date
	})
	fmt.Println("\n📋 Daily Log:")
	for _, e := range entries {
		dist := float64(e.Steps) * cfg.StrideCm / 100000.0
		cal := float64(e.Steps) * 0.04 * cfg.WeightKg / 70.0
		pct := int(float64(e.Steps) / float64(cfg.Goal) * 100)
		if pct > 100 {
			pct = 100
		}
		fmt.Printf("%s | %d steps | %.2f km | %.0f kcal | %d%%\n", e.Date, e.Steps, dist, cal, pct)
	}
}

func exportCSV(entries []Entry, cfg Config, filename string) {
	f, err := os.Create(filename)
	if err != nil {
		fmt.Println("Error creating file:", err)
		return
	}
	defer f.Close()
	f.WriteString("Date,Steps,Distance (km),Calories,Goal %\n")
	for _, e := range entries {
		dist := float64(e.Steps) * cfg.StrideCm / 100000.0
		cal := float64(e.Steps) * 0.04 * cfg.WeightKg / 70.0
		pct := int(float64(e.Steps) / float64(cfg.Goal) * 100)
		if pct > 100 {
			pct = 100
		}
		f.WriteString(fmt.Sprintf("%s,%d,%.2f,%.0f,%d\n", e.Date, e.Steps, dist, cal, pct))
	}
	fmt.Printf("✅ Exported %d entries to %s\n", len(entries), filename)
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: pedometer [config|add|stats|list|export]")
		return
	}
	cfg := loadConfig()
	entries := loadEntries()
	cmd := os.Args[1]

	switch cmd {
	case "config":
		configCmd := flag.NewFlagSet("config", flag.ExitOnError)
		stride := configCmd.Float64("stride", 0, "Stride length in cm")
		weight := configCmd.Float64("weight", 0, "Weight in kg")
		goal := configCmd.Int("goal", 0, "Daily step goal")
		configCmd.Parse(os.Args[2:])
		if *stride > 0 {
			cfg.StrideCm = *stride
		}
		if *weight > 0 {
			cfg.WeightKg = *weight
		}
		if *goal > 0 {
			cfg.Goal = *goal
		}
		saveConfig(cfg)
		fmt.Printf("✅ Config updated: stride=%.1fcm, weight=%.1fkg, goal=%d\n", cfg.StrideCm, cfg.WeightKg, cfg.Goal)

	case "add":
		if len(os.Args) < 3 {
			fmt.Println("Usage: add <steps> [--date YYYY-MM-DD]")
			return
		}
		steps, _ := strconv.Atoi(os.Args[2])
		dateStr := ""
		for i := 3; i < len(os.Args); i++ {
			if os.Args[i] == "--date" && i+1 < len(os.Args) {
				dateStr = os.Args[i+1]
				i++
			}
		}
		entries = addSteps(entries, steps, dateStr, cfg)

	case "stats":
		stats(entries, cfg)

	case "list":
		listEntries(entries, cfg)

	case "export":
		filename := "steps.csv"
		if len(os.Args) >= 3 {
			filename = os.Args[2]
		}
		exportCSV(entries, cfg, filename)

	default:
		fmt.Println("Unknown command")
	}
}
