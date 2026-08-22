// Pedometer.java
import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.util.*;
import com.google.gson.*;

class Config {
    double stride_cm = 78.0;
    double weight_kg = 70.0;
    int goal = 10000;
}

class Entry {
    String date;
    int steps;
}

public class Pedometer {
    private static final String CONFIG_FILE = "pedometer_config.json";
    private static final String DATA_FILE = "steps.json";
    private static final Gson gson = new GsonBuilder().setPrettyPrinting().create();

    public static void main(String[] args) throws Exception {
        if (args.length < 1) {
            System.out.println("Usage: Pedometer [config|add|stats|list|export]");
            return;
        }
        Config cfg = loadConfig();
        List<Entry> entries = loadEntries();
        String cmd = args[0];

        switch (cmd) {
            case "config":
                for (int i = 1; i < args.length; i++) {
                    if (args[i].equals("--stride") && i+1 < args.length) {
                        cfg.stride_cm = Double.parseDouble(args[++i]);
                    }
                    if (args[i].equals("--weight") && i+1 < args.length) {
                        cfg.weight_kg = Double.parseDouble(args[++i]);
                    }
                    if (args[i].equals("--goal") && i+1 < args.length) {
                        cfg.goal = Integer.parseInt(args[++i]);
                    }
                }
                saveConfig(cfg);
                System.out.printf("✅ Config updated: stride=%.1fcm, weight=%.1fkg, goal=%d\n", cfg.stride_cm, cfg.weight_kg, cfg.goal);
                break;

            case "add":
                if (args.length < 2) { System.out.println("Usage: add <steps> [--date YYYY-MM-DD]"); return; }
                int steps = Integer.parseInt(args[1]);
                String dateStr = null;
                for (int i = 2; i < args.length; i++) {
                    if (args[i].equals("--date") && i+1 < args.length) {
                        dateStr = args[++i];
                    }
                }
                entries = addSteps(entries, cfg, steps, dateStr);
                break;

            case "stats":
                stats(entries, cfg);
                break;

            case "list":
                listEntries(entries, cfg);
                break;

            case "export":
                String filename = args.length > 1 ? args[1] : "steps.csv";
                exportCSV(entries, cfg, filename);
                break;

            default:
                System.out.println("Unknown command");
        }
    }

    static Config loadConfig() throws IOException {
        Path path = Paths.get(CONFIG_FILE);
        if (!Files.exists(path)) return new Config();
        String json = new String(Files.readAllBytes(path));
        return gson.fromJson(json, Config.class);
    }

    static void saveConfig(Config cfg) throws IOException {
        Files.write(Paths.get(CONFIG_FILE), gson.toJson(cfg).getBytes());
    }

    static List<Entry> loadEntries() throws IOException {
        Path path = Paths.get(DATA_FILE);
        if (!Files.exists(path)) return new ArrayList<>();
        String json = new String(Files.readAllBytes(path));
        Entry[] arr = gson.fromJson(json, Entry[].class);
        return new ArrayList<>(Arrays.asList(arr));
    }

    static void saveEntries(List<Entry> entries) throws IOException {
        Files.write(Paths.get(DATA_FILE), gson.toJson(entries).getBytes());
    }

    static Entry getEntry(List<Entry> entries, String date) {
        for (Entry e : entries) {
            if (e.date.equals(date)) return e;
        }
        return null;
    }

    static List<Entry> addSteps(List<Entry> entries, Config cfg, int steps, String dateStr) throws IOException {
        if (dateStr == null) dateStr = LocalDate.now().toString();
        Entry e = getEntry(entries, dateStr);
        if (e != null) {
            e.steps = steps;
        } else {
            e = new Entry();
            e.date = dateStr;
            e.steps = steps;
            entries.add(e);
        }
        saveEntries(entries);
        double dist = steps * cfg.stride_cm / 100000.0;
        double cal = steps * 0.04 * cfg.weight_kg / 70.0;
        System.out.printf("✅ Logged %,d steps on %s – %.2f km, %.0f kcal\n", steps, dateStr, dist, cal);
        return entries;
    }

    static void stats(List<Entry> entries, Config cfg) {
        if (entries.isEmpty()) {
            System.out.println("No entries.");
            return;
        }
        int totalSteps = 0;
        for (Entry e : entries) totalSteps += e.steps;
        double totalDist = totalSteps * cfg.stride_cm / 100000.0;
        double totalCal = totalSteps * 0.04 * cfg.weight_kg / 70.0;
        int avgSteps = totalSteps / entries.size();
        String today = LocalDate.now().toString();
        Entry todayEntry = getEntry(entries, today);
        int todaySteps = todayEntry != null ? todayEntry.steps : 0;
        int pct = Math.min(100, (int)((double)todaySteps / cfg.goal * 100));
        double fuelSaved = totalDist / 10.0;
        double co2Saved = fuelSaved * 2.31;

        System.out.printf("\n🚶 Pedometer (Energy Saving)\n");
        System.out.printf("Today: %,d steps\n", todaySteps);
        System.out.printf("Goal: %,d steps (%d%% complete)\n", cfg.goal, pct);
        System.out.printf("\n📊 Statistics\n");
        System.out.printf("Total steps: %,d\n", totalSteps);
        System.out.printf("Total distance: %.2f km\n", totalDist);
        System.out.printf("Total calories: %.0f kcal\n", totalCal);
        System.out.printf("Average: %,d steps/day\n", avgSteps);
        System.out.printf("Energy saved: %.2f L of fuel\n", fuelSaved);
        System.out.printf("CO₂ saved: %.2f kg\n", co2Saved);
    }

    static void listEntries(List<Entry> entries, Config cfg) {
        if (entries.isEmpty()) {
            System.out.println("No entries.");
            return;
        }
        entries.sort((a, b) -> b.date.compareTo(a.date));
        System.out.println("\n📋 Daily Log:");
        for (Entry e : entries) {
            double dist = e.steps * cfg.stride_cm / 100000.0;
            double cal = e.steps * 0.04 * cfg.weight_kg / 70.0;
            int pct = Math.min(100, (int)((double)e.steps / cfg.goal * 100));
            System.out.printf("%s | %,d steps | %.2f km | %.0f kcal | %d%%\n", e.date, e.steps, dist, cal, pct);
        }
    }

    static void exportCSV(List<Entry> entries, Config cfg, String filename) throws IOException {
        try (BufferedWriter writer = Files.newBufferedWriter(Paths.get(filename))) {
            writer.write("Date,Steps,Distance (km),Calories,Goal %\n");
            for (Entry e : entries) {
                double dist = e.steps * cfg.stride_cm / 100000.0;
                double cal = e.steps * 0.04 * cfg.weight_kg / 70.0;
                int pct = Math.min(100, (int)((double)e.steps / cfg.goal * 100));
                writer.write(String.format("%s,%d,%.2f,%.0f,%d\n", e.date, e.steps, dist, cal, pct));
            }
        }
        System.out.printf("✅ Exported %d entries to %s\n", entries.size(), filename);
    }
}
