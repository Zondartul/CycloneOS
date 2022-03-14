#pragma once
#include <iostream>
using std::ostream;

//stolen from http://www.cplusplus.com/forum/general/20554/
class dual_stream {
public:
	dual_stream(std::ostream& os1, std::ostream& os2);

	template<class T>
	dual_stream& operator<<(const T& x) {
		os1 << x;
		os2 << x;
		return *this;
	}
	typedef ostream& (*ostreamManip)(ostream&);
	dual_stream& operator<<(ostreamManip manip);
	void flush();
private:
	std::ostream& os1;
	std::ostream& os2;
};
