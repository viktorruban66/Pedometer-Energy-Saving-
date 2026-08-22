# pedometer.py
import json
import os
import argparse
from datetime import datetime, date

CONFIG_FILE = "pedometer_config.json"
DATA_FILE = "steps.json"

class Pedometer:
    def __init__(self):
        self.stride_cm = 78.0
        self.weight_kg = 70.0
        self.goal = 10000
        self.entries = []
        self.load_config()
        self.load_entries()

    def load_config(self):
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, "r") as f:
                cfg = json.load(f)
                self.stride_cm = cfg.get("stride_cm", 78.0)
                self.weight_kg = cfg.get("weight_kg", 70.0)
                self.goal = cfg.get("goal", 10000)

    def save_config(self):
        with open(CONFIG_FILE, "w") as f:
            json.dump({
                "stride_cm": self.stride_cm,
                "weight_kg": self.weight_kg,
                "goal": self.goal
            }, f)

    def load_entries(self):
        if os.path.exists(DATA_FILE):
            with open(DATA_FILE, "r") as f:
                self.entries = json.load(f)

    def save_entries(self):
        with open(DATA_FILE, "w") as f:
            json.dump(self.entries, f, indent=2)

    def get_entry(self, date_str):
        for e in self.entries:
            if e["date"] == date_str:
                return e
        return None

    def add_steps(self, steps, date_str=None):
        if date_str is None:
            date_str = datetime.now().strftime("%Y-%m-%d")
        entry = self.get_entry(date_str)
        if entry:
            entry["steps"] = steps
        else:
            self.entries.append({"date": date_str, "steps": steps})
        self.save_entries()
        dist = (steps * self.stride_cm) / 100000.0  # km
        cal = steps * 0.04 * self.weight_kg / 70.0  # rough estimate
        print(f"✅ Logged {steps:,} steps on {date_str} – {dist:.2f} km, {cal:.0f} kcal")

    def stats(self):
        if not self.entries:
            print("No entries.")
            return
        total_steps = sum(e["steps"] for e in self.entries)
        total_dist = (total_steps * self.stride_cm) / 100000.0
        total_cal = total_steps * 0.04 * self.weight_kg / 70.0
        avg_steps = total_steps // len(self.entries)
        today = datetime.now().strftime("%Y-%m-%d")
        today_entry = self.get_entry(today)
        today_steps = today_entry["steps"] if today_entry else 0
        pct = min(100, int(today_steps / self.goal * 100))

        # Energy saving: walking vs driving (approx: 1 L fuel per 10 km)
        fuel_saved = total_dist / 10.0
        co2_saved = fuel_saved * 2.31  # kg CO2 per liter

        print(f"\n🚶 Pedometer (Energy Saving)")
        print(f"Today: {today_steps:,} steps")
        print(f"Goal: {self.goal:,} steps ({pct}% complete)")
        print(f"\n📊 Statistics")
        print(f"Total steps: {total_steps:,}")
        print(f"Total distance: {total_dist:.2f} km")
        print(f"Total calories: {total_cal:.0f} kcal")
        print(f"Average: {avg_steps:,} steps/day")
        print(f"Energy saved: {fuel_saved:.2f} L of fuel")
        print(f"CO₂ saved: {co2_saved:.2f} kg")

    def list(self):
        if not self.entries:
            print("No entries.")
            return
        print("\n📋 Daily Log:")
        for e in sorted(self.entries, key=lambda x: x["date"], reverse=True):
            steps = e["steps"]
            dist = (steps * self.stride_cm) / 100000.0
            cal = steps * 0.04 * self.weight_kg / 70.0
            pct = min(100, int(steps / self.goal * 100))
            print(f"{e['date']} | {steps:,} steps | {dist:.2f} km | {cal:.0f} kcal | {pct}%")

    def export(self, filename):
        import csv
        with open(filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(["Date", "Steps", "Distance (km)", "Calories", "Goal %"])
            for e in self.entries:
                steps = e["steps"]
                dist = (steps * self.stride_cm) / 100000.0
                cal = steps * 0.04 * self.weight_kg / 70.0
                pct = min(100, int(steps / self.goal * 100))
                writer.writerow([e["date"], steps, round(dist, 2), round(cal, 0), pct])
        print(f"✅ Exported {len(self.entries)} entries to {filename}")

def main():
    parser = argparse.ArgumentParser(description="Pedometer (Energy Saving)")
    subparsers = parser.add_subparsers(dest="cmd", required=True)

    config_parser = subparsers.add_parser("config")
    config_parser.add_argument("--stride", type=float, help="Stride length in cm")
    config_parser.add_argument("--weight", type=float, help="Weight in kg")
    config_parser.add_argument("--goal", type=int, help="Daily step goal")

    add_parser = subparsers.add_parser("add")
    add_parser.add_argument("steps", type=int)
    add_parser.add_argument("--date", help="YYYY-MM-DD")

    subparsers.add_parser("stats")
    subparsers.add_parser("list")

    export_parser = subparsers.add_parser("export")
    export_parser.add_argument("filename", default="steps.csv", nargs="?")

    args = parser.parse_args()
    ped = Pedometer()

    if args.cmd == "config":
        if args.stride:
            ped.stride_cm = args.stride
        if args.weight:
            ped.weight_kg = args.weight
        if args.goal:
            ped.goal = args.goal
        ped.save_config()
        print(f"✅ Config updated: stride={ped.stride_cm}cm, weight={ped.weight_kg}kg, goal={ped.goal}")
    elif args.cmd == "add":
        ped.add_steps(args.steps, args.date)
    elif args.cmd == "stats":
        ped.stats()
    elif args.cmd == "list":
        ped.list()
    elif args.cmd == "export":
        ped.export(args.filename)

if __name__ == "__main__":
    main()
