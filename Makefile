#
# Untwister Makefile
#

CC = g++
CPPFLAGS = -std=gnu++11 -O3 -g3 -Wall -c -fmessage-length=0 -MMD -fPIC
PYTHON = /usr/include/python2.7
BOOST = /usr/include
OBJS = ./prngs/LSBState.o ./prngs/GlibcRand.o ./prngs/PHP_mt19937.o ./prngs/Mt19937.o ./prngs/Ruby.o ./prngs/PRNGFactory.o ./Untwister.o
TEST_OBJS = ./tests/runner.o ./tests/TestRuby.o ./tests/TestMt19937.o ./tests/TestPRNGFactory.o ./tests/Test_PHP_mt19937.o ./tests/TestUntwister.o
CC = g++

all: GlibcRand Mt19937 PHP_mt19937 Ruby LSBState PRNGFactory Untwister
	$(CC) $(CPPFLAGS) -pthread -MF"main.d" -MT"main.d" -o "main.o" "./main.cpp"
	$(CC) -std=gnu++11 -O3 -pthread $(OBJS) main.o -o untwister

python: GlibcRand Mt19937 PHP_mt19937 Ruby LSBState PRNGFactory Untwister
	$(CC) $(CPPFLAGS) -I$(PYTHON) -I$(BOOST) py-untwister.cpp -o py-untwister.o
	$(CC) -std=c++11 -shared -fPIC -O3 $(OBJS) py-untwister.o -lboost_python -lpython2.7 -o untwister.so

tests: GlibcRand Mt19937 PHP_mt19937 Ruby LSBState PRNGFactory Untwister
	$(CC) $(CPPFLAGS) -MF"./tests/TestRuby.d" -MT"./tests/TestRuby.d" -o "./tests/TestRuby.o" "./tests/TestRuby.cpp"
	$(CC) $(CPPFLAGS) -MF"./tests/TestMt19937.d" -MT"./tests/TestMt19937.d" -o "./tests/TestMt19937.o" "./tests/TestMt19937.cpp"
	$(CC) $(CPPFLAGS) -MF"./tests/Test_PHP_mt19937.d" -MT"./tests/Test_PHP_mt19937.d" -o "./tests/Test_PHP_mt19937.o" "./tests/Test_PHP_mt19937.cpp"
	$(CC) $(CPPFLAGS) -MF"./tests/TestPRNGFactory.d" -MT"./tests/TestPRNGFactory.d" -o "./tests/TestPRNGFactory.o" "./tests/TestPRNGFactory.cpp"
	$(CC) $(CPPFLAGS) -MF"./tests/TestUntwister.d" -MT"./tests/TestUntwister.d" -o "./tests/TestUntwister.o" "./tests/TestUntwister.cpp"
	$(CC) $(CPPFLAGS) -MF"./tests/runner.d" -MT"./tests/runner.d" -o "./tests/runner.o" "./tests/runner.cpp"
	$(CC) -std=gnu++11 -O3 -pthread $(OBJS) $(TEST_OBJS) -o untwister_tests -lcppunit

# PRNG Objects
GlibcRand:
	$(CC) $(CPPFLAGS) -MF"prngs/GlibcRand.d" -MT"prngs/GlibcRand.d" -o "prngs/GlibcRand.o" "./prngs/GlibcRand.cpp"

Mt19937:
	$(CC) $(CPPFLAGS) -MF"prngs/Mt19937.d" -MT"prngs/Mt19937.d" -o "prngs/Mt19937.o" "./prngs/Mt19937.cpp"

PHP_mt19937:
	$(CC) $(CPPFLAGS) -MF"prngs/PHP_mt19937.d" -MT"prngs/PHP_mt19937.d" -o "prngs/PHP_mt19937.o" "./prngs/PHP_mt19937.cpp"

Ruby:
	$(CC) $(CPPFLAGS) -MF"prngs/Ruby.d" -MT"prngs/Ruby.d" -o "prngs/Ruby.o" "./prngs/Ruby.cpp"

LSBState:
	$(CC) $(CPPFLAGS) -MF"prngs/LSBState.d" -MT"prngs/LSBState.d" -o "prngs/LSBState.o" "./prngs/LSBState.cpp"

WPRand:
	$(CC) $(CPPFLAGS) -MF"prngs/WPRand.d" -MT"prngs/WPRand.d" -o "prngs/WPRand.o" "./prngs/WPRand.cpp"

# Other Stuff
Crypto:
	$(CC) $(CPPFLAGS) -MF"prngs/crypto/Md5.d" -MT"prngs/crypto/Md5.d" -o "prngs/crypto/Md5.o" "./prngs/crypto/Md5.cpp"
	$(CC) $(CPPFLAGS) -MF"prngs/crypto/Sha1.d" -MT"prngs/crypto/Sha1.d" -o "prngs/crypto/Sha1.o" "./prngs/crypto/Sha1.cpp"

PRNGFactory:
	$(CC) $(CPPFLAGS) -MF"prngs/PRNGFactory.d" -MT"prngs/PRNGFactory.d" -o "prngs/PRNGFactory.o" "./prngs/PRNGFactory.cpp"

Untwister:
	$(CC) $(CPPFLAGS) -pthread -MF"Untwister.d" -MT"Untwister.d" -o "Untwister.o" "./Untwister.cpp"

# Cleanup
clean:
	rm -f ./prngs/*.o
	rm -f ./prngs/*.d
	rm -f ./prngs/crypto/*.o
	rm -f ./prngs/crypto/*.d
	rm -f ./tests/*.o
	rm -f ./tests/*.d
	rm -f *.o
	rm -f *.d
	rm -f untwister untwister_tests untwister.so
