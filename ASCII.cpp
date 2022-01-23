#include <iostream>

using namespace std;

int main() {
	char a = 0;

	system("clear");
	cout << "Number | Hex | Char" << endl;
	for (unsigned short i = 1; i < 260; i++) {
		a = i;
		cout << dec << i << " | 0x" << hex << i << " | " << a << endl;
	}
	return 0;
}

