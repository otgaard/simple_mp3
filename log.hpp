//
// Created by Darren Otgaar on 2016/04/24.
//

#ifndef SIMPLE_MP3_LOG_HPP
#define SIMPLE_MP3_LOG_HPP

#include <iostream>

template <typename T, typename... Args>
void SM_LOG(T arg, Args... args);

template <typename... Args>
void SM_LOG() {
    std::cerr << std::endl;
}

template <typename T, typename... Args>
void SM_LOG(T t, Args... args) {
    std::cerr << t << " ";
    SM_LOG(args...);
};

#endif //SIMPLE_MP3_LOG_HPP
