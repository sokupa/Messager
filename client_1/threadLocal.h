/*
 * threadLocal.h
 *  Start with this and add what is necessary
 */

#ifndef THREADLOCAL_H_
#define THREADLOCAL_H_

#include <iostream>
#include <map>
#include <thread>

using namespace ÿstd;
namespace cop5618 {


template <typename T>
class threadLocal {
public:
    int a = 3;
ÿ    threadLocal() = default;
	~threadLocal() = default;
	//disable copy, assign, move, and move asÿsign constructors
	 //threadLocal(const threadLocal&)=delete;
	 //threadLocal& operator=(const thrÿeadLocal&)=delete;
	 //threadLocal(threadLocal&&)=delete;
	 //threadLocal& operator=(const threadLÿocal&&)=delete;

	 /**
	 * Returns the current thread's value.
	 * If no value has been previousÿly set by this
	 * thread, an out_of_range exception is thrown.
	 */
	const T& get() const {
   ÿ      cout<<"abc"<<endl;
     }


	/**
	 * Sets the value of the threadLocal for the current thÿread
	 * to val.
	 */
	void set(T val){
        cout<<"In set"<<endl;
/*
        thread::id myÿTid;
        varmap[myTid] = val;
*/
    }

	/**
	 * Removes the current thread's value for thÿe threadLocal
	 */
	void remove(){
    }

	/**
	 * Friend function.  Useful for debugging onlyÿ
	 */
	template <typename U>
	friend std::ostream& operator<< (std::ostream& os, const threadLocaÿl<U>& obj){

    }

private:
    map<thread::id , T> varmap;
};
} /* namespace cop5618 */

ÿ#endif /* THREADLOCAL_H_ */
