/*
	Preliminary C Task for Buutti Consulting Oy
	Created by: Olli Ahtola
*/
#include <stdio.h>

int if_prime(unsigned long long num);

int main(void)
{
	unsigned long long number_1, number_2;
	unsigned char bool_var = 0;
	
	number_1 = ((1000000000 / 6) + 1)*6 - 1; // Smallest 10 digit number that fulfill the condition 6n +- 1
	
	while(1) // Run until a prime is found
	{
		if(if_prime(number_1))
			break;
		else 
			number_1 += 2 + 2*bool_var; //Increment number under test fulfill the condition 6n +- 1
		bool_var ^= 1;
	}
	
	bool_var = 0;
	number_2 = ((10000000000 / 6) + 1)*6 - 1; // Smallest 10 digit number that fulfill the condition 6n +- 1
	
	while(1)
	{
		if(if_prime(number_2))
			break;
		else 
			number_2 += 2 + 2*bool_var;
		bool_var ^= 1;
	}
	
	printf("First 15 digits of the product of smallest 10 digit\nand 11 digit primes: %.15llu\n", (number_1*number_2)%1000000000000000);
	
	return 0;
}

/*Resolves if input number is prime. Assumes input number fulfills the condition: num = 6n +-1 */
int if_prime(unsigned long long num)
{
	for(unsigned long long divider = 6; divider*divider < num; divider += 6)
	{
		if(num % (divider - 1) == 0 || num % (divider + 1) == 0)
			return 0;
	}
	return 1;
}