//
//  main.cpp
//  tp2
//
//  Created by Hekuenra on 2017/10/14.
//  Copyright © 2017年 Hekuenra. All rights reserved.
//
#include <iostream>
#include "Tp2.h"

int main()
{
    d2::threadpool tp;
    std::mutex mtx;
    std::vector< std::future<int> > fucs_res;
    
    for(int i = 0; i < 5; i++ )
    {
        fucs_res.emplace_back( tp.entasks([i,&mtx](){
            std::lock_guard< std::mutex > lg(mtx);
            std::cout << "thread creadted" << std::endl;
            return i * i;
        }));
    }
    
    for( auto &res : fucs_res )
        res.get();
}
