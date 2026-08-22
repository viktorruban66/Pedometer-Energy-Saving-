// pedometer.js
#!/usr/bin/env node
const fs = require('fs');
const { program } = require('commander');

const CONFIG_FILE = 'pedometer_config.json';
const DATA_FILE = 'steps.json';

let cfg = { strideCm: 78, weightKg: 70, goal: 10000 };
let entries = [];

function loadConfig() {
    if (fs.existsSync(CONFIG_FILE)) {
        try {
            cfg = JSON.parse(fs.readFileSync(CONFIG_FILE));
        } catch (e) {}
    }
}

function saveConfig() {
    fs.writeFileSync(CONFIG_FILE, JSON.stringify(cfg, null, 2));
}

function loadEntries() {
    if (fs.existsSync(DATA_FILE)) {
        try {
            entries = JSON.parse(fs.readFileSync(DATA_FILE));
        } catch (e) {}
    }
}

function saveEntries() {
    fs.writeFileSync(DATA_FILE, JSON.stringify(entries, null, 2));
}

function getEntry(date) {
    return entries.find(e => e.date === date);
}

function formatSteps(steps) {
    return steps.toLocaleString();
}

program
    .command('config')
    .option('--stride <cm>', 'Stride length in cm', parseFloat)
    .option('--weight <kg>', 'Weight in kg', parseFloat)
    .option('--goal <n>', 'Daily step goal', parseInt)
    .action((options) => {
        if (options.stride) cfg.strideCm = options.stride;
        if (options.weight) cfg.weightKg = options.weight;
        if (options.goal) cfg.goal = options.goal;
        saveConfig();
        console.log(`✅ Config updated: stride=${cfg.strideCm}cm, weight=${cfg.weightKg}kg, goal=${cfg.goal}`);
    });

program
    .command('add <steps>')
    .option('--date <date>', 'YYYY-MM-DD')
    .action((steps, options) => {
        const s = parseInt(steps);
        const date = options.date || new Date().toISOString().slice(0,10);
        const entry = getEntry(date);
        if (entry) {
            entry.steps = s;
        } else {
            entries.push({ date, steps: s });
        }
        saveEntries();
        const dist = s * cfg.strideCm / 100000;
        const cal = s * 0.04 * cfg.weightKg / 70;
        console.log(`✅ Logged ${formatSteps(s)} steps on ${date} – ${dist.toFixed(2)} km, ${cal.toFixed(0)} kcal`);
    });

program
    .command('stats')
    .action(() => {
        if (!entries.length) {
            console.log('No entries.');
            return;
        }
        const totalSteps = entries.reduce((sum, e) => sum + e.steps, 0);
        const totalDist = totalSteps * cfg.strideCm / 100000;
        const totalCal = totalSteps * 0.04 * cfg.weightKg / 70;
        const avgSteps = Math.round(totalSteps / entries.length);
        const today = new Date().toISOString().slice(0,10);
        const todayEntry = getEntry(today);
        const todaySteps = todayEntry ? todayEntry.steps : 0;
        const pct = Math.min(100, Math.round(todaySteps / cfg.goal * 100));
        const fuelSaved = totalDist / 10;
        const co2Saved = fuelSaved * 2.31;

        console.log(`\n🚶 Pedometer (Energy Saving)`);
        console.log(`Today: ${formatSteps(todaySteps)} steps`);
        console.log(`Goal: ${formatSteps(cfg.goal)} steps (${pct}% complete)`);
        console.log(`\n📊 Statistics`);
        console.log(`Total steps: ${formatSteps(totalSteps)}`);
        console.log(`Total distance: ${totalDist.toFixed(2)} km`);
        console.log(`Total calories: ${totalCal.toFixed(0)} kcal`);
        console.log(`Average: ${formatSteps(avgSteps)} steps/day`);
        console.log(`Energy saved: ${fuelSaved.toFixed(2)} L of fuel`);
        console.log(`CO₂ saved: ${co2Saved.toFixed(2)} kg`);
    });

program
    .command('list')
    .action(() => {
        if (!entries.length) {
            console.log('No entries.');
            return;
        }
        const sorted = [...entries].sort((a, b) => b.date.localeCompare(a.date));
        console.log('\n📋 Daily Log:');
        for (const e of sorted) {
            const dist = e.steps * cfg.strideCm / 100000;
            const cal = e.steps * 0.04 * cfg.weightKg / 70;
            const pct = Math.min(100, Math.round(e.steps / cfg.goal * 100));
            console.log(`${e.date} | ${formatSteps(e.steps)} steps | ${dist.toFixed(2)} km | ${cal.toFixed(0)} kcal | ${pct}%`);
        }
    });

program
    .command('export [filename]')
    .action((filename = 'steps.csv') => {
        let csv = 'Date,Steps,Distance (km),Calories,Goal %\n';
        for (const e of entries) {
            const dist = e.steps * cfg.strideCm / 100000;
            const cal = e.steps * 0.04 * cfg.weightKg / 70;
            const pct = Math.min(100, Math.round(e.steps / cfg.goal * 100));
            csv += `${e.date},${e.steps},${dist.toFixed(2)},${cal.toFixed(0)},${pct}\n`;
        }
        fs.writeFileSync(filename, csv);
        console.log(`✅ Exported ${entries.length} entries to ${filename}`);
    });

program.parse(process.argv);
