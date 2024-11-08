#ifndef STATISTICSDB_H
#define STATISTICSDB_H


class StatisticsDB
{
public:
    StatisticsDB();
    ~StatisticsDB();

private:
    bool createDbConnection();
};

#endif // STATISTICSDB_H
