#include "pch.h"
#include <iostream>
#include <vector>
#include <algorithm>

// Datum interface
struct Datum
{
	virtual bool operator<(const Datum& other) const = 0;
};

struct StringDatum : Datum
{
	std::string str;

	StringDatum(std::string _str)
	{
		str = std::move(_str);
	}

	bool operator<(const StringDatum& other) const
	{
		return str < other.str;
	}

	virtual bool operator<(const Datum& other) const override
	{
		return *this < static_cast<const StringDatum&>(other);
	}
};

struct NumDatum : Datum
{
	double num;

	NumDatum(double _num)
	{
		num = _num;
	}

	bool operator<(const NumDatum& other) const
	{
		return num < other.num;
	}

	virtual bool operator<(const Datum& other) const override
	{
		return *this < static_cast<const NumDatum&>(other);
	}
};

struct Entry
{
	std::vector<std::unique_ptr<Datum>> data;

	// Assume first Datum is the primary key
	Entry(std::vector<std::unique_ptr<Datum>> _data)
	{
		data = std::move(_data);
	}

	Datum* getPrimaryKey() const
	{
		return data[0].get();
	}
};

struct Table
{
	std::vector<Entry> entries;
	std::vector<Datum*> index;

	Table(std::vector<Entry> _entries)
	{
		entries = std::move(_entries);
		createIndex();
	}

	void createIndex()
	{
		std::sort(entries.begin(), entries.end(), [](const Entry& first, const Entry& second)
		{
			return *first.getPrimaryKey() < *second.getPrimaryKey();
		});
		index.reserve(entries.size());
		for (Entry& entry : entries)
		{
			index.push_back(entry.getPrimaryKey());
		}
	}
};

void serialize(const Table& table)
{
	// TODO
}

int main()
{
	std::vector<std::unique_ptr<Datum>> tValues;
	tValues.push_back(std::make_unique<NumDatum>(3));
	tValues.push_back(std::make_unique<NumDatum>(2));
	tValues.push_back(std::make_unique<StringDatum>("Test"));
	tValues.push_back(std::make_unique<NumDatum>(10));

	Entry tEntry(std::move(tValues));
	std::vector<Entry> tEntries;
	tEntries.push_back(std::move(tEntry));

	Table tTable(std::move(tEntries));
}