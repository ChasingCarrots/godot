//
// Created by martin on 20.02.26.
//

#ifndef MASTER_BLENDALOT_MATH_HELPER_H
#define MASTER_BLENDALOT_MATH_HELPER_H

inline int greatest_common_divisor(int a, int b) {
	while (b != 0) {
		int temp = b;
		b = a % b;
		a = temp;
	}

	return a;
}

inline int least_common_multiple(int a, int b) {
	return (a / greatest_common_divisor(a, b)) * b;
}

#endif //MASTER_BLENDALOT_MATH_HELPER_H
