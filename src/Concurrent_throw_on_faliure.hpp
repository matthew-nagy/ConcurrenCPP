//When this flag is on, should a concurrency class notice some deadlock or issue that would occour while
//being deleted, it will throw an error. This is bad form and will set off warnings in most compilers, but
//I view this as better than letting the program continue in an nstable state
#define CONCURENT_LIB_THROW_ON_FALIURE

//Include this file to message out that it should fail