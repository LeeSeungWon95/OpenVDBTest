#include <openvdb/openvdb.h>
#include <iostream>
#include <openvdb/io/File.h>
#include <openvdb/math/Transform.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

static inline void ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
		{
			return !std::isspace(ch);
		}));
}
static inline void rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch)
		{
			return !std::isspace(ch); }).base(), s.end());
}
static inline void trim(std::string& s) { ltrim(s); rtrim(s); }

static inline void stripQuotes(std::string& s)
{
	if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
	{
		s = s.substr(1, s.size() - 2);
	}
}

static inline void stripBOM(std::string& s)
{
	if (s.size() >= 3 && (unsigned char)s[0] == 0xEF && (unsigned char)s[1] == 0xBB && (unsigned char)s[2] == 0xBF)
	{
		s.erase(0, 3);
	}
}

std::vector<std::string> splitCSVSimple(const std::string& line)
{
	std::vector<std::string> out;
	std::string cur;
	for (char c : line)
	{
		if (c == ',')
		{
			out.push_back(cur);
			cur.clear();
		}
		else
			cur.push_back(c);
	}
	out.push_back(cur);
	return out;
}

void makeVDBFile(openvdb::math::Transform::Ptr InTrans, openvdb::Vec3fGrid::Accessor InAcc, openvdb::Vec3fGrid::Ptr InGrid,
	std::string File, std::string OutPut)
{
	std::string Path = "C:\\DevLee\\VDBTest2\\";
	Path.append(File);

	std::ifstream ifs(Path);
	if (!ifs)
	{
		std::cerr << "CSV not found\n";
		return;
	}

	std::string line;
	if (!std::getline(ifs, line))
	{
		std::cerr << "Empty CSV\n";
		return;
	}

	auto cols = splitCSVSimple(line);

	if (!cols.empty()) stripBOM(cols[0]);

	for (auto& c : cols)
	{
		trim(c);
		stripQuotes(c);
	}

	for (auto& c : cols)
		std::cout << "[" << c << "]\n";

	auto colIndex = [&](const std::string& name)->int
		{
			for (int i = 0; i < (int)cols.size(); ++i)
			{
				if (cols[i] == name)
					return i;
			}
			return -1;
		};
	int ix = colIndex("Points:0");
	int iy = colIndex("Points:1");
	int iz = colIndex("Points:2");

	int ipmv = colIndex("PMV");

	if (ix < 0 || iy < 0 || iz < 0)
	{
		std::cerr << "Required columns not found\n";
		return;
	}

	std::vector<std::string> tokens;
	tokens.reserve(cols.size());

	size_t lineCount = 0;
	float MaxValue = FLT_MIN;
	float MinValue = FLT_MAX;

	int count = 0;

	std::vector<float> pmvs;
	std::vector<openvdb::Vec3d> Locations;

	while (std::getline(ifs, line))
	{
		if (line.empty())
			continue;
		tokens.clear();
		{
			std::stringstream ss(line);
			std::string tok;
			while (std::getline(ss, tok, ','))
				tokens.push_back(tok);
		}
		if ((int)tokens.size() != (int)cols.size())
			continue;

		double x = std::stod(tokens[ix]);
		double y = std::stod(tokens[iy]);
		double z = std::stod(tokens[iz]);
		Locations.push_back(openvdb::Vec3d(x, y, z));

		float pmv = std::stof(tokens[ipmv]);
		pmvs.push_back(pmv);
		MinValue = std::min(pmv, MinValue);
		MaxValue = std::max(pmv, MaxValue);

		count++;


	}

	for (int i = 0; i < count; ++i)
	{
		openvdb::Coord ijk = InTrans->worldToIndexCellCentered(Locations[i]);

		InAcc.setValueOn(ijk, openvdb::Vec3f(pmvs[i], MinValue, MaxValue));
	}

	

	if (++lineCount % 200'000 == 0)
		std::cout << "processed " << lineCount << "rows\n";

	openvdb::io::File file(OutPut);
	std::vector<openvdb::GridBase::Ptr> grids;
	grids.push_back(InGrid);
	file.write(grids);
	file.close();
}


int main()
{
	openvdb::initialize();

	const double voxelSize = 0.1f;
	openvdb::math::Transform::Ptr xform = openvdb::math::Transform::createLinearTransform(voxelSize);
	openvdb::Vec3fGrid::Ptr PMVGrid = openvdb::Vec3fGrid::create(openvdb::Vec3f(0.f, 0.f, 0.f));
	PMVGrid->setGridClass(openvdb::GRID_FOG_VOLUME);
	PMVGrid->setName("PMV");
	PMVGrid->setTransform(xform);



	openvdb::Vec3fGrid::Accessor accessor = PMVGrid->getAccessor();

	for (int i = 0; i < 4; ++i)
	{
		std::string FileName = "merged.csv";
		std::size_t pos = FileName.find('.');
		std::string first = FileName.substr(0, pos);
		std::string rest = FileName.substr(pos);
		first.append(std::to_string(i + 1));
		first.append(rest);
		
		std::string OutputName = "OutPut.vdb";
		pos = OutputName.find('.');
		std::string Trimmed = OutputName.substr(0, pos);
		rest = OutputName.substr(pos);
		Trimmed.append("_000");
		Trimmed.append(std::to_string(i + 1));
		Trimmed.append(rest);

		makeVDBFile(xform, accessor, PMVGrid, first, Trimmed);
	}

	std::cout << "Wrote out.vdb\n";

	return 0;
}