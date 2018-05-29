//
// Created by Peter Zheng on 2018/05/28.
//

#ifndef INC_2018DATASTRUCTUREBIGWORK_DATEANDTIME_H
#define INC_2018DATASTRUCTUREBIGWORK_DATEANDTIME_H

#include "CoreData.h"
#include "String.h"

namespace myAlgorithm {
    class Date {
    private:
        TriplePair<short, short, short> dataOfDate;
    public:
        Date() = default;

        Date(short year, short month, short day) {
            dataOfDate.setData(year, month, day);
        }

        Date(const Date &rhs) {
            dataOfDate = rhs.dataOfDate;
        }

        Date &operator=(const Date &rhs) {
            if (&rhs == this) return *this;
            dataOfDate.setData(rhs.getYear(), rhs.getMonth(), rhs.getDay());
            return *this;
        }

        const short getYear() const {
            return dataOfDate.first();
        }

        const short getMonth() const {
            return dataOfDate.second();
        }

        const short getDay() const {
            return dataOfDate.third();
        }

        inline bool M_isLeapYear() const {
            return (dataOfDate.first() % 4 == 0 && dataOfDate.first() % 100 != 0) || (dataOfDate.first() % 400 == 0);
        }

        inline bool M_isLeapYear(int year) const {
            return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        }

        inline int M_dayInTheYear() const {
            short day = dataOfDate.third();
            short month = dataOfDate.second();
            short Day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (M_isLeapYear()) Day[1] = 29;
            for (int i = 0; i < month - 1; i++) day += Day[i];
            return day;
        }

        inline int M_dayInTheYear(int year, int month, int day) const {
            int Day[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (M_isLeapYear(year)) Day[1] = 29;
            for (int i = 0; i < month - 1; i++) day += Day[i];
            return day;
        }

        int operator-(const Date &rhs) const {
            int year1 = dataOfDate.first();
            int month1 = dataOfDate.second();
            int day1 = dataOfDate.third();
            int year2 = rhs.getYear();
            int month2 = rhs.getMonth();
            int day2 = rhs.getDay();
            if (year1 == year2 && month1 == month2)
                return day1 > day2 ? day1 - day2 : day2 - day1;
            if (year1 == year2) {
                int d1, d2;
                d1 = M_dayInTheYear();
                d2 = rhs.M_dayInTheYear();
                return d1 > d2 ? d1 - d2 : d2 - d1;
            }
            if (year1 > year2) {
                int t;
                t = year1;
                year1 = year2;
                year2 = t;
                t = month1;
                month1 = month2;
                month2 = t;
                t = day1;
                day1 = day2;
                day2 = t;
            }
            int d1, d2, d3;
            if (M_isLeapYear())
                d1 = 366 - M_dayInTheYear(year1, month1, day1);
            else
                d1 = 365 - M_dayInTheYear(year1, month1, day1);
            d2 = rhs.M_dayInTheYear(year2, month2, day2);
            d3 = 0;
            for (int i = year1 + 1; i < year2; i++)
                if (M_isLeapYear(i)) d3 += 366; else d3 += 365;
            return d1 + d2 + d3;
        }

        friend std::ostream &operator<<(std::ostream &os, const Date &data) {
            os << data.getYear() << "-" << data.getMonth() << "-" << data.getDay();
            return os;
        }

        friend std::istream &operator>>(std::istream &is, Date &data) {
            short t1, t2, t3;
            char ch, ch2;
            is >> t1 >> ch >> t2 >> ch2 >> t3;
            return is;
        }
    };

    class Time {
    private:
        Pair<short, short> dataOfTime;
    public:
        Time() { dataOfTime = Pair<short, short>(0, 0); }

        Time(const Time &rhs) : dataOfTime(rhs.dataOfTime) {}

        Time(const Time &rhs, int AddTime) {
            dataOfTime = rhs.dataOfTime;
            int min = dataOfTime.second();
            int hour = dataOfTime.first();
            min += AddTime;
            if (min < 0) {
                hour -= 100;
                min += 6000;
            }
            hour += min / 60;
            min = min % 60;
            while(hour < 0) hour += 24;
            dataOfTime.setData(hour, min);
        }

        Time(int hh, int mm) : dataOfTime(Pair<short, short>(hh, mm)) {}

        Time(const String& source){
            String s1, s2;
            bool Result;
            split(':', source, s1, s2, String::__SPLIT_STRING_WITHOUT_CHAR, Result);
            if(!Result) dataOfTime = Pair<short, short>(0, 0);
            else {
                int res1 = (int) s1;
                int res2 = (int) s2;
                if(res1 < 0 || res1 > 24 || res2 < 0 || res2 > 60) dataOfTime = Pair<short, short>(0, 0);
                else dataOfTime = Pair<short, short>(res1, res2);
            }
        }

        Time(const String& source, int AddTime){
            String s1, s2;
            bool Result;
            split(':', source, s1, s2, String::__SPLIT_STRING_WITHOUT_CHAR, Result);
            if(!Result) dataOfTime = Pair<short, short>(0, 0);
            else {
                int res1 = (int) s1;
                int res2 = (int) s2;
                if(res1 < 0 || res1 > 24 || res2 < 0 || res2 > 60) dataOfTime = Pair<short, short>(0, 0);
                else dataOfTime = Pair<short, short>(res1, res2);
            }
            int min = dataOfTime.second();
            int hour = dataOfTime.first();
            min += AddTime;
            if (min < 0) {
                hour -= 100;
                min += 6000;
            }
            hour += min / 60;
            min = min % 60;
            while(hour < 0) hour += 24;
            dataOfTime.setData(hour, min);
        }
    };
}

#endif //INC_2018DATASTRUCTUREBIGWORK_DATEANDTIME_H