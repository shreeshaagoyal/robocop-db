#include "pch.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>

template<typename T>
void WriteToBuffer(std::vector<char>& buffer, const T& value);
template<>
void WriteToBuffer(std::vector<char>& buffer, const std::string& str);
template<>
void WriteToBuffer(std::vector<char>& buffer, const std::vector<char>& value);
template<typename T>
void WriteToFile(std::ofstream& ofs, const T& value);
template<>
void WriteToFile(std::ofstream& ofs, const std::vector<char>& value);

// Datum interface
struct Datum
{
	virtual bool operator<(const Datum& other) const = 0;
	virtual size_t GetHash() const = 0;
	virtual uint8_t GetType() const = 0;
	virtual const std::vector<char>& GetBinary() const = 0;

protected:
	std::vector<char> bin;
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

	virtual size_t GetHash() const override
	{
		return std::hash<std::string>{}(str);
	}

	virtual uint8_t GetType() const override
	{
		return 0;
	}

	virtual const std::vector<char>& GetBinary() const override
	{
		auto& _bin = const_cast<std::vector<char>&>(bin);
		_bin.clear();
		WriteToBuffer(_bin, str.size());
		WriteToBuffer(_bin, str);
		return bin;
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

	virtual size_t GetHash() const override
	{
		return std::hash<double>{}(num);
	}

	virtual uint8_t GetType() const override
	{
		return 1;
	}

	virtual const std::vector<char>& GetBinary() const override
	{
		auto& _bin = const_cast<std::vector<char>&>(bin);
		_bin.clear();
		WriteToBuffer(_bin, num);
		return bin;
	}
};

struct Entry
{
	std::vector<std::unique_ptr<Datum>> data;
	std::vector<char> bin;

	// Assume first Datum is the primary key
	Entry(std::vector<std::unique_ptr<Datum>> _data)
	{
		data = std::move(_data);
	}

	Datum* GetPrimaryKey() const
	{
		return data[0].get();
	}

	const std::vector<char>& GetBinary() const
	{
		auto& _bin = const_cast<std::vector<char>&>(bin);
		_bin.clear();

		for (const auto& datum : data)
		{
			const auto& datumBin = datum->GetBinary();
			WriteToBuffer(_bin, datumBin);
		}

		return bin;
	}
};

struct Table
{
	std::vector<Entry> entries;
	std::vector<Datum*> index;

	Table(std::vector<Entry> _entries)
	{
		entries = std::move(_entries);
		CreateIndex();
	}

	void CreateIndex()
	{
		std::sort(entries.begin(), entries.end(), [](const Entry& first, const Entry& second)
		{
			return first.GetPrimaryKey()->GetHash() < second.GetPrimaryKey()->GetHash();
		});
		index.reserve(entries.size());
		for (Entry& entry : entries)
		{
			index.push_back(entry.GetPrimaryKey());
		}
	}

	// Index will contain hash values
	// Entries will contain original values
	void Serialize(const std::string& filepath)
	{
		std::ofstream ofs{ filepath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc };

		if (entries.empty())
			return;

		// Serialize index
		for (auto* indexEntry : index)
		{
			WriteToFile(ofs, indexEntry->GetHash());
			WriteToFile(ofs, static_cast<uint64_t>(0));
		}

		// Serialize entries

		// number of columns in a single entry
		// order of num/string Datums in a single Entry (e.g. {num,num,string})
		// List of entries:
		// If a Datum is a StringDatum, indicate the length before the actual string

		const auto& cols = entries[0].data;
		WriteToFile(ofs, cols.size());

		for (const auto& col : cols)
		{
			WriteToFile(ofs, col->GetType());
		}

		uint64_t entryIndex = 0;

		for (const auto& entry : entries)
		{
			auto filePos = static_cast<uint64_t>(ofs.tellp());

			{
				// TODO remove magic numbers
				ofs.seekp(entryIndex * 16 + 8);
				WriteToFile(ofs, filePos);
				ofs.seekp(filePos);
			}

			WriteToFile(ofs, entry.GetBinary());
			++entryIndex;
		}
	}
};

template<typename T>
void WriteToBuffer(std::vector<char>& buffer, const T& value)
{
	for (int i = 0; i < sizeof(value); ++i)
	{
		buffer.push_back(reinterpret_cast<const char*>(&value)[i]);
	}
}

template<>
void WriteToBuffer(std::vector<char>& buffer, const std::string& str)
{
	for (int i = 0; i < str.size(); ++i)
	{
		buffer.push_back(str[i]);
	}
}

template<>
void WriteToBuffer(std::vector<char>& buffer, const std::vector<char>& value)
{
	buffer.insert(buffer.end(), value.begin(), value.end());
}

template<typename T>
void WriteToFile(std::ofstream& ofs, const T& value)
{
	ofs.write(reinterpret_cast<char*>(&(const_cast<T&>(value))), sizeof(value));
}

template<>
void WriteToFile(std::ofstream& ofs, const std::vector<char>& value)
{
	ofs.write(value.data(), value.size());
}

int main()
{
	std::vector<std::unique_ptr<Datum>> tValues;
	tValues.push_back(std::make_unique<NumDatum>(3));
	tValues.push_back(std::make_unique<NumDatum>(2));
	tValues.push_back(std::make_unique<StringDatum>("Test1"));
	tValues.push_back(std::make_unique<NumDatum>(10));

	Entry tEntry(std::move(tValues));
	std::vector<Entry> tEntries;
	tEntries.push_back(std::move(tEntry));

	tValues.push_back(std::make_unique<NumDatum>(5));
	tValues.push_back(std::make_unique<NumDatum>(1));
	tValues.push_back(std::make_unique<StringDatum>("Test2"));
	tValues.push_back(std::make_unique<NumDatum>(6));

	tEntry = Entry(std::move(tValues));
	tEntries.push_back(std::move(tEntry));

	Table tTable(std::move(tEntries));
	tTable.Serialize("table.bin");
}