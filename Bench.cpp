#include <thread>
#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/timer/timer.hpp>

#include <ctime>

typedef std::string DataFilename;
typedef std::string Key;
typedef std::string Value;

void executeProgram(std::string s)
{
	std::cout << s << std::endl;
    system(s.c_str());
}

std::string generateOutputFilename(std::string exec, std::string data_file)
{
	std::hash<std::string> hash_fn;
	std::string s = std::to_string(hash_fn(exec + data_file));

	std::string s_tmp = boost::filesystem::path(exec).string();
	std::replace_if(s_tmp.begin(), s_tmp.end(),
		[](char c) -> bool { return !std::isalnum(c); }, '-');
	s += "_" + s_tmp;

	s_tmp = boost::filesystem::path(data_file).string();
	std::replace_if(s_tmp.begin(), s_tmp.end(),
		[](char c) -> bool { return !std::isalnum(c); }, '-');
	s += "_" + s_tmp + ".ini";
	return s;
}

int main(int argc, char *argv[])
{
	// Parse program options
	std::string exec = "";
	std::string output_file = "";
	time_t time_limit = 0;
	boost::program_options::options_description desc("Allowed options");
	desc.add_options()
		("help,h", "produce help message")
		("input-data,i",  boost::program_options::value<std::vector<std::string>>()->required()->multitoken(), "set input data (required)")
		("output-file,o", boost::program_options::value<std::string>(&output_file)->required(), "set output file (required)")
		("exec,e",        boost::program_options::value<std::string>(&exec)->required(), "set exec (required)")
		("time-limit,t",  boost::program_options::value<time_t>(&time_limit), "set time limit")
		;
	boost::program_options::variables_map vm;
	boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
	if (vm.count("help")) {
		std::cout << desc << std::endl;;
		return 1;
	}
	try {
		boost::program_options::notify(vm);
	} catch (boost::program_options::required_option e) {
		std::cout << e.what() << std::endl;
		std::cout << desc << std::endl;;
		return 1;
	}

	// Run algorithm
	for (std::string data_file: vm["input-data"].as<std::vector<std::string>>()) {
		std::string output = generateOutputFilename(exec, data_file);
		if (boost::filesystem::exists(output))
			continue;
		std::string cmd = exec
				+ " -i " + data_file
				+ " -o " + output;
		boost::timer::cpu_timer timer;
		std::thread worker(executeProgram, cmd);
		worker.join();
		boost::timer::cpu_times elapsed = timer.elapsed();
		if (!boost::filesystem::exists(output))
			continue;
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini(output, pt);
		pt.put("Algorithm.CPU time", (elapsed.user + elapsed.system) / 1e9);
		pt.put("Algorithm.WC time", elapsed.wall / 1e9);
		write_ini(output, pt);
	}

	// Read output files
	std::vector<Key> keys;
	keys.push_back("CPU time");
	keys.push_back("WC time");
	std::map<std::tuple<DataFilename, Key>, Value> res;
	for (std::string data_file: vm["input-data"].as<std::vector<std::string>>()) {
		std::string output = generateOutputFilename(exec, data_file);
		if (!boost::filesystem::exists(output))
			continue;
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini(output, pt);
		for (auto& sec: pt) {
			for (auto& kv: sec.second) {
				std::string key   = kv.first;
				std::string value = kv.second.get_value<std::string>();
				if (std::find(keys.begin(), keys.end(), key) == keys.end())
					keys.push_back(key);
				res[std::make_tuple(data_file, key)] = value;
			}
		}
	}

	// Write CSV file
	std::ofstream out;
	out.open(output_file);
	out << "," << exec << std::endl;
	// First Line
	out << "Data";
	for (Key key: keys)
		out << "," << key;
	out << std::endl;
	// Other lines
	for (std::string data_file: vm["input-data"].as<std::vector<std::string>>()) {
		out << data_file;
		for (Key key: keys)
			out << "," << res[std::make_tuple(data_file, key)];
		out << std::endl;
	}
	out.close();

	return 0;
}
