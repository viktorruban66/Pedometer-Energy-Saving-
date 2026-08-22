# pedometer.rb
#!/usr/bin/env ruby
require 'json'
require 'date'

CONFIG_FILE = 'pedometer_config.json'
DATA_FILE = 'steps.json'

class Pedometer
  attr_reader :stride_cm, :weight_kg, :goal, :entries

  def initialize
    @stride_cm = 78.0
    @weight_kg = 70.0
    @goal = 10000
    @entries = []
    load_config
    load_entries
  end

  def load_config
    if File.exist?(CONFIG_FILE)
      cfg = JSON.parse(File.read(CONFIG_FILE))
      @stride_cm = cfg['stride_cm'] || 78.0
      @weight_kg = cfg['weight_kg'] || 70.0
      @goal = cfg['goal'] || 10000
    end
  end

  def save_config
    File.write(CONFIG_FILE, JSON.pretty_generate({
      'stride_cm' => @stride_cm,
      'weight_kg' => @weight_kg,
      'goal' => @goal
    }))
  end

  def load_entries
    if File.exist?(DATA_FILE)
      @entries = JSON.parse(File.read(DATA_FILE))
    end
  end

  def save_entries
    File.write(DATA_FILE, JSON.pretty_generate(@entries))
  end

  def get_entry(date)
    @entries.find { |e| e['date'] == date }
  end

  def add_steps(steps, date_str = nil)
    date_str ||= Date.today.to_s
    entry = get_entry(date_str)
    if entry
      entry['steps'] = steps
    else
      @entries << { 'date' => date_str, 'steps' => steps }
    end
    save_entries
    dist = steps * @stride_cm / 100000.0
    cal = steps * 0.04 * @weight_kg / 70.0
    puts "✅ Logged #{steps.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse} steps on #{date_str} – #{'%.2f' % dist} km, #{cal.round(0)} kcal"
  end

  def stats
    if @entries.empty?
      puts "No entries."
      return
    end
    total_steps = @entries.sum { |e| e['steps'] }
    total_dist = total_steps * @stride_cm / 100000.0
    total_cal = total_steps * 0.04 * @weight_kg / 70.0
    avg_steps = total_steps / @entries.size
    today = Date.today.to_s
    today_entry = get_entry(today)
    today_steps = today_entry ? today_entry['steps'] : 0
    pct = [100, (today_steps.to_f / @goal * 100).to_i].min
    fuel_saved = total_dist / 10.0
    co2_saved = fuel_saved * 2.31

    puts "\n🚶 Pedometer (Energy Saving)"
    puts "Today: #{today_steps.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse} steps"
    puts "Goal: #{@goal.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse} steps (#{pct}% complete)"
    puts "\n📊 Statistics"
    puts "Total steps: #{total_steps.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse}"
    puts "Total distance: #{'%.2f' % total_dist} km"
    puts "Total calories: #{total_cal.round(0)} kcal"
    puts "Average: #{avg_steps.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse} steps/day"
    puts "Energy saved: #{'%.2f' % fuel_saved} L of fuel"
    puts "CO₂ saved: #{'%.2f' % co2_saved} kg"
  end

  def list
    if @entries.empty?
      puts "No entries."
      return
    end
    sorted = @entries.sort_by { |e| e['date'] }.reverse
    puts "\n📋 Daily Log:"
    sorted.each do |e|
      steps = e['steps']
      dist = steps * @stride_cm / 100000.0
      cal = steps * 0.04 * @weight_kg / 70.0
      pct = [100, (steps.to_f / @goal * 100).to_i].min
      puts "#{e['date']} | #{steps.to_s.reverse.gsub(/(\d{3})(?=\d)/, '\\1,').reverse} steps | #{'%.2f' % dist} km | #{cal.round(0)} kcal | #{pct}%"
    end
  end

  def export(filename)
    require 'csv'
    CSV.open(filename, 'w') do |csv|
      csv << ["Date", "Steps", "Distance (km)", "Calories", "Goal %"]
      @entries.each do |e|
        steps = e['steps']
        dist = steps * @stride_cm / 100000.0
        cal = steps * 0.04 * @weight_kg / 70.0
        pct = [100, (steps.to_f / @goal * 100).to_i].min
        csv << [e['date'], steps, dist.round(2), cal.round(0), pct]
      end
    end
    puts "✅ Exported #{@entries.size} entries to #{filename}"
  end
end

if ARGV.empty?
  puts "Usage: pedometer.rb [config|add|stats|list|export]"
  exit
end

ped = Pedometer.new
cmd = ARGV.shift

case cmd
when 'config'
  while ARGV.any?
    case ARGV[0]
    when '--stride'
      ARGV.shift
      ped.stride_cm = ARGV.shift.to_f
    when '--weight'
      ARGV.shift
      ped.weight_kg = ARGV.shift.to_f
    when '--goal'
      ARGV.shift
      ped.goal = ARGV.shift.to_i
    else
      break
    end
  end
  ped.save_config
  puts "✅ Config updated: stride=#{ped.stride_cm}cm, weight=#{ped.weight_kg}kg, goal=#{ped.goal}"

when 'add'
  if ARGV.empty?
    puts "Usage: add <steps> [--date YYYY-MM-DD]"
    exit
  end
  steps = ARGV.shift.to_i
  date_str = nil
  if ARGV.include?('--date')
    idx = ARGV.index('--date')
    date_str = ARGV[idx+1] if idx
  end
  ped.add_steps(steps, date_str)

when 'stats'
  ped.stats

when 'list'
  ped.list

when 'export'
  filename = ARGV.shift || 'steps.csv'
  ped.export(filename)

else
  puts "Unknown command"
end
