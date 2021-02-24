#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "./cpp-httplib/httplib.h"
#include <stdio.h>
#include <sstream>
#include "influxdb.hpp"
#include <boost/property_tree/json_parser.hpp>
#include "../keyfiles/secret.h"


#define CERT_FILE "../keyfiles/server.crt"
#define KEY_FILE "../keyfiles/server.key"


int main() {
	using namespace httplib;
	using namespace std;
	SSLServer svr(CERT_FILE,KEY_FILE);
	influxdb_cpp::server_info si("127.0.0.1",8086, "data");

	std::cout << "------------------ ESP SERVER ----------------" << std::endl;

	svr.Get("/text", [](const Request &req, Response &res) {
			std::cout << "Request received" << std::endl;
			res.set_content("Hello World", "text/plain");
			});

	svr.Post("/submit", [&si](const Request &req, Response &res) {
			int devid(0);
			std::stringstream received_string;
			received_string << req.body;
			string secret = SECRET;
			boost::property_tree::ptree pt;
			try {
				boost::property_tree::read_json(received_string, pt);
				devid = std::stoi(pt.get<std::string>("devid"));
				if(pt.get<std::string>("key") != secret) {
					throw;
				}
			} catch(...) {
				res.set_content("Failed to parse json", "text/html");
				return;
			}
		
			char s[11];
			sprintf(s,"%ld", devid);

			std::cout << "Device Id: " << devid << std::endl;
			for(boost::property_tree::ptree::value_type &measurement : pt.get_child("measurements")) {	
			std::cout << "Inserting Property Name: " << measurement.first << " | With value: " << measurement.second.data() << std::endl;
				
				int success =  influxdb_cpp::builder()
					.meas(measurement.first)
					.tag("devid", s)
					.field("value", std::stoi(measurement.second.data()), 1)
					.post_http(si);
				std::cout << "Write code: " << success << std::endl;
				if(success != 0) { std::cout << "Error writing to database: " << success << std::endl; }
			}
			std::cout << std::endl << std::endl;
			
			res.set_content("success", "text/html");



			return;
			});
	
	svr.listen("0.0.0.0", 4445);
	return 0;

}
