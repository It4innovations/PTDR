#pragma once

#include <iterator>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

namespace Routing {
    class CSVReader {
    public:
        /**
         * Constructor, accepts separator char, defaults to semicolon
         * @param separator
         */
        CSVReader(char separator = ';') : m_separator(separator) {}

        /**
         * Array access operator overload for column access for current row
         * @param index is a idx of column to read
         * @return value in the selected column as string
         */
        std::string const &operator[](std::size_t index) const;

        /**
         * Size of current row
         * @return number of columns in current row
         */
        std::size_t size() const;

        /**
         * Fetches next row from file
         * @param str is an input stream of the file to be read
         */
        void readNextRow(std::istream &str);

        /**
         * Stream read operator overload for convenient access to the file rows
         * @param str input stream of the CSV file
         * @param data current instance of the reader
         * @return input stream to the CSV file
         */
        friend std::istream &operator>>(std::istream &str, CSVReader &data);

    private:
        /**
         * Hold current parsed row
         */
        std::vector<std::string> m_data;

        /**
         * Current CSV separator
         */
        const char m_separator;
    };
}
