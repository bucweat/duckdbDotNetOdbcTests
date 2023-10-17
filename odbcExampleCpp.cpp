#include "pch.h"
#include <iostream>
#include <string>
#include <exception>
#include <cstdarg>
#include <vector>

using namespace System;
using namespace System::Data;
using namespace System::Data::Odbc;

//*************************************************************
static std::string toss(System::String^ s)
{
    using namespace Runtime::InteropServices;
    // convert .NET System::String to std::string
    const char* cstr = (const char*)(Marshal::StringToHGlobalAnsi(s)).ToPointer();
    std::string sstr = cstr;
    Marshal::FreeHGlobal(System::IntPtr((void*)cstr));
    return sstr;
}

//*************************************************************
// requires at least C++11
const std::string vformat(const char* const zcFormat, ...) {

    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, zcFormat);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, zcFormat, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), zcFormat, vaArgs);
    va_end(vaArgs);
    return std::string(zc.data(), iLen);
}

//*************************************************************
std::string doReader(OdbcConnection^ DbConnection, System::String^ query)
{
    std::string s("");
    std::string t("");
    s += vformat("\ndoReader> %s %d\n", query, DbConnection->State);

    OdbcCommand^ DbCommand = DbConnection->CreateCommand();
    DbCommand->CommandText = query;
    try
    {
        OdbcDataReader^ DbReader = DbCommand->ExecuteReader();
        do
        {
            int fCount = DbReader->FieldCount;
            if (fCount > 0)
            {
                s += ":";
                for (int i = 0; i < fCount; i++)
                {
                    s += toss(DbReader->GetName(i));
                }
                s += "\n";
                while (DbReader->Read())
                {
                    s += ":";
                    for (int i = 0; i < fCount; i++)
                    {
                        s += toss(DbReader->GetString(i));
                    }
                    s += "\n";
                }
            }
            else
            {
                int row_count = DbReader->RecordsAffected;
                s += vformat("Query affected %d row(s)\n", row_count);
            }
        } while (DbReader->NextResult());
        DbReader->Close();
    }
    catch (OdbcException^ ex)
    {
        std::string e("");
        e += toss(ex->Message);
        return e;
    }

    delete DbCommand;

    return s += "\n";
}

//*************************************************************
int main(array<System::String ^> ^args)
{
    int32_t returnVal = 0;
    std::string s ("");

    System::String ^ connStr = "Driver=DuckDB Driver;Database=test.duckdb;";
    OdbcConnection ^ DbConnection = gcnew OdbcConnection(connStr);

    std::cout << "starting..." << std::endl;
    
    try
    {
        DbConnection->Open();

        std::cout << "\n-----------------------------------\nBuild DB\n" << std::endl;

        s = doReader(DbConnection, "PRAGMA version;");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "select * from sqlite_master;");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "drop table if exists weather;");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "select * from sqlite_master;");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "CREATE TABLE weather(city VARCHAR, temp_lo INTEGER, temp_hi INTEGER, prcp FLOAT, date DATE);");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "INSERT INTO weather VALUES ('San Francisco', 46, 50, 0.25, '1994-11-27');");
        std::cout << s << std::endl;

        s = doReader(DbConnection, "select temp_lo from weather limit 1;");
        std::cout << s << std::endl;

        std::cout << "-----------------------------------\n" << std::endl;

        DataTable ^ dt = gcnew DataTable();
        OdbcDataAdapter ^ adapter = gcnew OdbcDataAdapter("select temp_lo from weather limit 1;", DbConnection);
        s = "\n-----------------------------------\n";
        s += "FillSchema()\n";
        std::cout << s << std::endl;

        adapter->FillSchema(dt, SchemaType::Source);

        s = "-----------------------------------\n";
        std::cout << s << std::endl;

    }
    catch (OdbcException^ ex)
    {
        std::cout << "****************************************************************" << std::endl;
        std::cout << "OdbcException: " << toss(ex->Message) << std::endl << std::endl << toss(ex->StackTrace) << std::endl;
        returnVal = -1;
    }
    catch (Exception^ ex)
    {
        std::cout << "****************************************************************" << std::endl;
        std::cout << "Exception:" << toss(ex->Message) << std::endl << std::endl << toss(ex->StackTrace) << std::endl;
        returnVal = -1;
    }
    finally
    {
        DbConnection->Close();
    }

    std::cout << s << std::endl;
    // std::cout << "press any key to continue..." << std::endl;
    // Console::ReadLine();

    return returnVal;
}

