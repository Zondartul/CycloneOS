#include "dual_stream.h"


dual_stream::dual_stream(std::ostream& os1, std::ostream& os2) : os1(os1), os2(os2) {}

dual_stream& dual_stream::operator<<(dual_stream::ostreamManip manip) {
	manip(os1);
	manip(os2);
	return *this;
}

void dual_stream::flush() {
	os1.flush();
	os2.flush();
}