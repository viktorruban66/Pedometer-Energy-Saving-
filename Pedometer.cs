// Pedometer.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class Config
{
    [JsonPropertyName("stride_cm")]
    public double StrideCm { get; set; } = 78.0;
    [JsonPropertyName("weight_kg")]
    public double WeightKg { get; set; } = 70.0;
    [JsonPropertyName("goal")]
    public int Goal { get; set; } = 10000;
}

class Entry
{
    [JsonPropertyName("date")]
    public string Date { get; set; }
    [JsonPropertyName("steps")]
    public int Steps { get; set; }
}

class Pedometer
{
    private static readonly string ConfigFile = "pedometer_config.json";
    private static readonly string DataFile = "steps.json";
    private static readonly JsonSerializerOptions Options = new JsonSerializerOptions { WriteIndented = true };

    static void Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: Pedometer [config|add|stats|list|export]");
            return;
        }
        Config cfg = LoadConfig();
        List<Entry> entries = LoadEntries();
        string cmd = args[0];

        switch (cmd)
        {
            case "config":
                for (int i = 1; i < args.Length; i++)
                {
                    if (args[i] == "--stride" && i+1 < args.Length)
                        cfg.StrideCm = double.Parse(args[++i]);
                    if (args[i] == "--weight" && i+1 < args.Length)
                        cfg.WeightKg = double.Parse(args[++i]);
                    if (args[i] == "--goal" && i+1 < args.Length)
                        cfg.Goal = int.Parse(args[++i]);
                }
                SaveConfig(cfg);
                Console.WriteLine($"✅ Config updated: stride={cfg.StrideCm}cm, weight={cfg.WeightKg}kg, goal={cfg.Goal}");
                break;

            case "add":
                if (args.Length < 2) { Console.WriteLine("Usage: add <steps> [--date YYYY-MM-DD]"); return; }
                int steps = int.Parse(args[1]);
                string dateStr = null;
                for (int i = 2; i < args.Length; i++)
                {
                    if (args[i] == "--date" && i+1 < args.Length)
                        dateStr = args[++i];
                }
                entries = AddSteps(entries, cfg, steps, dateStr);
                break;

            case "stats":
                Stats(entries, cfg);
                break;

            case "list":
                ListEntries(entries, cfg);
                break;

            case "export":
                string filename = args.Length > 1 ? args[1] : "steps.csv";
                ExportCSV(entries, cfg, filename);
                break;

            default:
                Console.WriteLine("Unknown command");
                break;
        }
    }

    static Config LoadConfig()
    {
        if (!File.Exists(ConfigFile)) return new Config();
        string json = File.ReadAllText(ConfigFile);
        return JsonSerializer.Deserialize<Config>(json) ?? new Config();
    }

    static void SaveConfig(Config cfg)
    {
        string json = JsonSerializer.Serialize(cfg, Options);
        File.WriteAllText(ConfigFile, json);
    }

    static List<Entry> LoadEntries()
    {
        if (!File.Exists(DataFile)) return new List<Entry>();
        string json = File.ReadAllText(DataFile);
        return JsonSerializer.Deserialize<List<Entry>>(json) ?? new List<Entry>();
    }

    static void SaveEntries(List<Entry> entries)
    {
        string json = JsonSerializer.Serialize(entries, Options);
        File.WriteAllText(DataFile, json);
    }

    static Entry GetEntry(List<Entry> entries, string date)
    {
        return entries.FirstOrDefault(e => e.Date == date);
    }

    static List<Entry> AddSteps(List<Entry> entries, Config cfg, int steps, string dateStr)
    {
        if (string.IsNullOrEmpty(dateStr)) dateStr = DateTime.Now.ToString("yyyy-MM-dd");
        var entry = GetEntry(entries, dateStr);
        if (entry != null)
        {
            entry.Steps = steps;
        }
        else
        {
            entries.Add(new Entry { Date = dateStr, Steps = steps });
        }
        SaveEntries(entries);
        double dist = steps * cfg.StrideCm / 100000.0;
        double cal = steps * 0.04 * cfg.WeightKg / 70.0;
        Console.WriteLine($"✅ Logged {steps:N0} steps on {dateStr} – {dist:F2} km, {cal:F0} kcal");
        return entries;
    }

    static void Stats(List<Entry> entries, Config cfg)
    {
        if (!entries.Any())
        {
            Console.WriteLine("No entries.");
            return;
        }
        int totalSteps = entries.Sum(e => e.Steps);
        double totalDist = totalSteps * cfg.StrideCm / 100000.0;
        double totalCal = totalSteps * 0.04 * cfg.WeightKg / 70.0;
        int avgSteps = totalSteps / entries.Count;
        string today = DateTime.Now.ToString("yyyy-MM-dd");
        var todayEntry = GetEntry(entries, today);
        int todaySteps = todayEntry?.Steps ?? 0;
        int pct = Math.Min(100, (int)((double)todaySteps / cfg.Goal * 100));
        double fuelSaved = totalDist / 10.0;
        double co2Saved = fuelSaved * 2.31;

        Console.WriteLine($"\n🚶 Pedometer (Energy Saving)");
        Console.WriteLine($"Today: {todaySteps:N0} steps");
        Console.WriteLine($"Goal: {cfg.Goal:N0} steps ({pct}% complete)");
        Console.WriteLine($"\n📊 Statistics");
        Console.WriteLine($"Total steps: {totalSteps:N0}");
        Console.WriteLine($"Total distance: {totalDist:F2} km");
        Console.WriteLine($"Total calories: {totalCal:F0} kcal");
        Console.WriteLine($"Average: {avgSteps:N0} steps/day");
        Console.WriteLine($"Energy saved: {fuelSaved:F2} L of fuel");
        Console.WriteLine($"CO₂ saved: {co2Saved:F2} kg");
    }

    static void ListEntries(List<Entry> entries, Config cfg)
    {
        if (!entries.Any())
        {
            Console.WriteLine("No entries.");
            return;
        }
        var sorted = entries.OrderByDescending(e => e.Date);
        Console.WriteLine("\n📋 Daily Log:");
        foreach (var e in sorted)
        {
            double dist = e.Steps * cfg.StrideCm / 100000.0;
            double cal = e.Steps * 0.04 * cfg.WeightKg / 70.0;
            int pct = Math.Min(100, (int)((double)e.Steps / cfg.Goal * 100));
            Console.WriteLine($"{e.Date} | {e.Steps:N0} steps | {dist:F2} km | {cal:F0} kcal | {pct}%");
        }
    }

    static void ExportCSV(List<Entry> entries, Config cfg, string filename)
    {
        using var writer = new StreamWriter(filename);
        writer.WriteLine("Date,Steps,Distance (km),Calories,Goal %");
        foreach (var e in entries)
        {
            double dist = e.Steps * cfg.StrideCm / 100000.0;
            double cal = e.Steps * 0.04 * cfg.WeightKg / 70.0;
            int pct = Math.Min(100, (int)((double)e.Steps / cfg.Goal * 100));
            writer.WriteLine($"{e.Date},{e.Steps},{dist:F2},{cal:F0},{pct}");
        }
        Console.WriteLine($"✅ Exported {entries.Count} entries to {filename}");
    }
}
