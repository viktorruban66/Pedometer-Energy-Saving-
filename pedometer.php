# pedometer.php
#!/usr/bin/env php
<?php

define('CONFIG_FILE', 'pedometer_config.json');
define('DATA_FILE', 'steps.json');

function loadConfig() {
    $cfg = ['stride_cm' => 78.0, 'weight_kg' => 70.0, 'goal' => 10000];
    if (file_exists(CONFIG_FILE)) {
        $data = json_decode(file_get_contents(CONFIG_FILE), true);
        if ($data) {
            $cfg = array_merge($cfg, $data);
        }
    }
    return $cfg;
}

function saveConfig($cfg) {
    file_put_contents(CONFIG_FILE, json_encode($cfg, JSON_PRETTY_PRINT));
}

function loadEntries() {
    if (file_exists(DATA_FILE)) {
        return json_decode(file_get_contents(DATA_FILE), true) ?: [];
    }
    return [];
}

function saveEntries($entries) {
    file_put_contents(DATA_FILE, json_encode($entries, JSON_PRETTY_PRINT));
}

function getEntry($entries, $date) {
    foreach ($entries as $e) {
        if ($e['date'] == $date) return $e;
    }
    return null;
}

function addSteps(&$entries, $cfg, $steps, $dateStr = null) {
    if (!$dateStr) $dateStr = date('Y-m-d');
    $entry = getEntry($entries, $dateStr);
    if ($entry) {
        $entry['steps'] = $steps;
    } else {
        $entries[] = ['date' => $dateStr, 'steps' => $steps];
    }
    saveEntries($entries);
    $dist = $steps * $cfg['stride_cm'] / 100000.0;
    $cal = $steps * 0.04 * $cfg['weight_kg'] / 70.0;
    echo "✅ Logged " . number_format($steps) . " steps on $dateStr – " . number_format($dist, 2) . " km, " . round($cal) . " kcal\n";
}

if ($argc < 2) {
    die("Usage: php pedometer.php [config|add|stats|list|export]\n");
}

$cmd = $argv[1];
$cfg = loadConfig();
$entries = loadEntries();

switch ($cmd) {
    case 'config':
        for ($i=2; $i<$argc; $i++) {
            if ($argv[$i] == '--stride' && isset($argv[$i+1])) {
                $cfg['stride_cm'] = (float)$argv[++$i];
            }
            if ($argv[$i] == '--weight' && isset($argv[$i+1])) {
                $cfg['weight_kg'] = (float)$argv[++$i];
            }
            if ($argv[$i] == '--goal' && isset($argv[$i+1])) {
                $cfg['goal'] = (int)$argv[++$i];
            }
        }
        saveConfig($cfg);
        echo "✅ Config updated: stride={$cfg['stride_cm']}cm, weight={$cfg['weight_kg']}kg, goal={$cfg['goal']}\n";
        break;

    case 'add':
        if ($argc < 3) die("Usage: add <steps> [--date YYYY-MM-DD]\n");
        $steps = (int)$argv[2];
        $dateStr = null;
        for ($i=3; $i<$argc; $i++) {
            if ($argv[$i] == '--date' && isset($argv[$i+1])) {
                $dateStr = $argv[++$i];
            }
        }
        addSteps($entries, $cfg, $steps, $dateStr);
        break;

    case 'stats':
        if (empty($entries)) {
            echo "No entries.\n";
            break;
        }
        $totalSteps = array_sum(array_column($entries, 'steps'));
        $totalDist = $totalSteps * $cfg['stride_cm'] / 100000.0;
        $totalCal = $totalSteps * 0.04 * $cfg['weight_kg'] / 70.0;
        $avgSteps = floor($totalSteps / count($entries));
        $today = date('Y-m-d');
        $todayEntry = getEntry($entries, $today);
        $todaySteps = $todayEntry ? $todayEntry['steps'] : 0;
        $pct = min(100, round($todaySteps / $cfg['goal'] * 100));
        $fuelSaved = $totalDist / 10.0;
        $co2Saved = $fuelSaved * 2.31;

        echo "\n🚶 Pedometer (Energy Saving)\n";
        echo "Today: " . number_format($todaySteps) . " steps\n";
        echo "Goal: " . number_format($cfg['goal']) . " steps ($pct% complete)\n";
        echo "\n📊 Statistics\n";
        echo "Total steps: " . number_format($totalSteps) . "\n";
        echo "Total distance: " . number_format($totalDist, 2) . " km\n";
        echo "Total calories: " . round($totalCal) . " kcal\n";
        echo "Average: " . number_format($avgSteps) . " steps/day\n";
        echo "Energy saved: " . number_format($fuelSaved, 2) . " L of fuel\n";
        echo "CO₂ saved: " . number_format($co2Saved, 2) . " kg\n";
        break;

    case 'list':
        if (empty($entries)) {
            echo "No entries.\n";
            break;
        }
        usort($entries, function($a, $b) { return strcmp($b['date'], $a['date']); });
        echo "\n📋 Daily Log:\n";
        foreach ($entries as $e) {
            $steps = $e['steps'];
            $dist = $steps * $cfg['stride_cm'] / 100000.0;
            $cal = $steps * 0.04 * $cfg['weight_kg'] / 70.0;
            $pct = min(100, round($steps / $cfg['goal'] * 100));
            echo "{$e['date']} | " . number_format($steps) . " steps | " . number_format($dist, 2) . " km | " . round($cal) . " kcal | $pct%\n";
        }
        break;

    case 'export':
        $filename = $argv[2] ?? 'steps.csv';
        $fp = fopen($filename, 'w');
        fputcsv($fp, ['Date', 'Steps', 'Distance (km)', 'Calories', 'Goal %']);
        foreach ($entries as $e) {
            $steps = $e['steps'];
            $dist = $steps * $cfg['stride_cm'] / 100000.0;
            $cal = $steps * 0.04 * $cfg['weight_kg'] / 70.0;
            $pct = min(100, round($steps / $cfg['goal'] * 100));
            fputcsv($fp, [$e['date'], $steps, round($dist, 2), round($cal, 0), $pct]);
        }
        fclose($fp);
        echo "✅ Exported " . count($entries) . " entries to $filename\n";
        break;

    default:
        echo "Unknown command\n";
}
?>
