/*
 * Time.hh
 *
 *  Created on: Jun 19, 2011
 *      Author: yuasa
 */

#ifndef TIME_HH_
#define TIME_HH_

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

namespace CxxUtilities {
class Time {
public:
	/** Returns the current date/time formatted accordingly.
	 */
	static std::string getCurrentTimeAsString(std::string format ="%Y-%m-%d %I:%M:%S") {
		time_t timer = time(NULL);
		struct tm *date;
		char str[1024];
		date = localtime(&timer);
		strftime(str, 1023, format.c_str(), date);
		return std::string(str);
	}

	static time_t getUNIXTime(){
		return time(0);
	}
};
}
#endif /* TIME_HH_ */
