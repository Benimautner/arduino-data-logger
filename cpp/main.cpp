#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./cpp-httplib/httplib.h"
#include <stdio.h>
#include <sstream>
#include <utility>
#include <vector>
#include "influxdb.hpp"
#include <boost/property_tree/json_parser.hpp>
#include "../keyfiles/secret.h"
#define ERR_MISSING_FILES_CORR "Run cmd.sh in /keyfiles and check that you're executing from the right directory"

#define CERT_FILE "../keyfiles/server.crt"
#define KEY_FILE "../keyfiles/server.key"

bool test_if_file_exists(const std::string& name) {
    struct stat buf;
    return (stat (name.c_str(), &buf) == 0);
}


int main() {
	using namespace httplib;
	using namespace std;
	SSLServer svr(CERT_FILE,KEY_FILE);
	influxdb_cpp::server_info si("127.0.0.1",8086, "data");

	map<int, pair<time_t, string>> last_data;

	std::cout << "------------------ ESP SERVER ----------------" << std::endl;
	if(!test_if_file_exists(CERT_FILE)) {
	    std::cout << "Couldn't find CERT file." << std::endl
	    << ERR_MISSING_FILES_CORR << std::endl;
	    return 1;
	}
	if(!test_if_file_exists(KEY_FILE)) {
	    std::cout << "Couldn't find KEY file." << std::endl
	    << ERR_MISSING_FILES_CORR << std::endl;
	    return 1;
	}

	svr.Get("/text", [](const Request &req, Response &res) {
			std::cout << "Request received" << std::endl;
			res.set_content("Hello World", "text/plain");
			});


	svr.Get("/status", [&last_data](const Request &req, Response &res) {
			string response_string("");
			for(auto data_point : last_data) {
				response_string += "---------------------\n";
				int devid = data_point.first;
				auto data = data_point.second;
				response_string += "Devid: " + to_string(devid) + "\n";
				response_string += (string)std::ctime(&data.first)+ "\n"+ data.second;
				response_string += "\n\n";
			}
			res.set_content(response_string, "text/plain");

			});

	// request time and values of latest data point with devid
	svr.Get("/query", [&last_data](const Request &req, Response &res) {
			//get devid from request
			int devid = stoi((req.params.find("devid")->second));
			//get point of device from public map
			auto last_point = last_data[devid]; 

			//return time and point
			res.set_content((string)std::ctime(&last_point.first)+ "\n\r"+ last_point.second, "application/json");	
			});

	svr.Post("/submit", [&si, &last_data](const Request &req, Response &res) {
			int devid(0);					
			std::stringstream received_string;					// stringstream containing request body (data)
			received_string << req.body;						// putting data into stringstream
			string secret = SECRET;							// creating string with constant secret
			boost::property_tree::ptree pt;						// property tree is almost like a dictionary (key & value pairs) 
			try {
				boost::property_tree::read_json(received_string, pt);		// filling property tree
				devid = std::stoi(pt.get<std::string>("devid"));
				if(pt.get<std::string>("key") != secret) {
					throw;
				}

				//string contains what we want to save
				auto temp_str = received_string.str();		

				//removing the key to avoid leaking
				auto pos = temp_str.find(secret);
				temp_str.erase(pos, secret.length());
				temp_str.insert(pos, "removed");

				//get current time and save it alongside the data
				time_t time = std::time(nullptr);
				last_data[devid] = pair<time_t, string> (time, temp_str);
				//last_data.insert(std::pair<int,std::string>(devid, received_string.str()));
			} catch(...) {
				res.set_content("Failed to parse json", "text/html");
				return;
			}
		
			char s[11];
			sprintf(s,"%d", devid);

			std::cout << "Device Id: " << devid << std::endl;
			for(boost::property_tree::ptree::value_type &measurement : pt.get_child("measurements")) {	
				std::cout << "Inserting Property Name: " << measurement.first << " | With value: " << measurement.second.data() << std::endl;
				
				int success =  influxdb_cpp::builder()
					.meas(measurement.first)
					.tag("devid", s)
					.field("value", std::stod(measurement.second.data()), 2)
					.post_http(si);
				std::cout << "Write code: " << success << std::endl;
				if(success != 0) { std::cout << "Error writing to database: " << success << std::endl; }
			}
			std::cout << std::endl << std::endl;
			
			res.set_content("success", "text/html");


			return;
			});
	
	svr.listen("0.0.0.0", 4445);
	std::cout << "Server Stopped" << std::endl;
	return 0;

}
