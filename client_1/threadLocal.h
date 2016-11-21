/*
 * threadLocal.h
 *  Start with this and add what is necessary
 */

#ifndef THREADLOCAL_H_
#define THREADLOCAL_H_

#include <iostream>
#include <map>
#include <thread>

using namespace �std;
namespace cop5618 {


template <typename T>
class threadLocal {
public:
    int a = 3;
�    threadLocal() = default;
	~threadLocal() = default;
	//disable copy, assign, move, and move as�sign constructors
	 //threadLocal(const threadLocal&)=delete;
	 //threadLocal& operator=(const thr�eadLocal&)=delete;
	 //threadLocal(threadLocal&&)=delete;
	 //threadLocal& operator=(const threadL�ocal&&)=delete;

	 /**
	 * Returns the current thread's value.
	 * If no value has been previous�ly set by this
	 * thread, an out_of_range exception is thrown.
	 */
	const T& get() const {
   �      cout<<"abc"<<endl;
     }


	/**
	 * Sets the value of the threadLocal for the current th�read
	 * to val.
	 */
	void set(T val){
        cout<<"In set"<<endl;
/*
        thread::id my�Tid;
        varmap[myTid] = val;
*/
    }

	/**
	 * Removes the current thread's value for th�e threadLocal
	 */
	void remove(){
    }

	/**
	 * Friend function.  Useful for debugging only�
	 */
	template <typename U>
	friend std::ostream& operator<< (std::ostream& os, const threadLoca�l<U>& obj){

    }

private:
    map<thread::id , T> varmap;
};
} /* namespace cop5618 */

�#endif /* THREADLOCAL_H_ */
