#include <iostream>
#include "httplib.h"

int main() {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("<h1>Timetable Generator is running!</h1>", "text/html");
    });

    std::cout << "Server started at http://localhost:8080\n";
    svr.listen("localhost", 8080);
}