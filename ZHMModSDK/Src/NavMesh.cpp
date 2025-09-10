#include "NavPower.h"

uint32_t RangeCheck(uint32_t val, uint32_t min, uint32_t max) {
	if (val > max) return max;
	if (val < min) return min;
	return val;
}

namespace NavPower
{
	void FixAreaPointers(uintptr_t data, size_t areaBytes)
	{
		uintptr_t navGraphStart = data;
		uintptr_t curIndex = data + sizeof(Binary::NavGraphHeader);
		size_t areaEndPtr = curIndex + areaBytes;

		while (curIndex != areaEndPtr)
		{
			Binary::Area* curArea = reinterpret_cast<Binary::Area*>(curIndex);
			curIndex += sizeof(Binary::Area);
			for (uint32_t i = 0; i < curArea->m_flags.GetNumEdges(); i++)
			{
				Binary::Edge* curEdge = (Binary::Edge*) curIndex;
				curIndex += sizeof(Binary::Edge);

				Binary::Area* adjArea = curEdge->m_pAdjArea;
				if (adjArea != NULL)
					curEdge->m_pAdjArea = (Binary::Area*) (navGraphStart + (char*) adjArea);
			}
		}
	}

	/*SVector3 Area::CalculateCentroid() {
		SVector3 normal = CalculateNormal();
		SVector3 v0 = m_edges.at(0)->m_pos;
		SVector3 v1 = m_edges.at(1)->m_pos;

		SVector3 u = (v1 - v0).GetUnitVec();
		SVector3 v = u.Cross(normal).GetUnitVec();

		std::vector<SVector3> mappedPoints;
		for (Binary::Edge* edge: m_edges) {
			SVector3 relativePos = edge->m_pos - v0;
			float uCoord = relativePos.Dot(u);
			float vCoord = relativePos.Dot(v);
			SVector3 uvv = SVector3(uCoord, vCoord, 0.0);
			mappedPoints.push_back(uvv);
		}
		float sum = 0;
		for (int i = 0; i < mappedPoints.size(); i++) {
			int nextI = (i + 1) % mappedPoints.size();
			sum += mappedPoints[i].X * mappedPoints[nextI].Y - mappedPoints[nextI].X * mappedPoints[i].Y;
		}
		float area = sum / 2;
		if (area < 0) {
			area *= -1;
		}

		float sumX = 0;
		float sumY = 0;
		for (int i = 0; i < mappedPoints.size(); i++) {
			int nextI = (i + 1) % mappedPoints.size();
			float x0 = mappedPoints[i].X;
			float x1 = mappedPoints[nextI].X;
			float y0 = mappedPoints[i].Y;
			float y1 = mappedPoints[nextI].Y;

			float doubleArea = (x0 * y1) - (x1 * y0);
			sumX += (x0 + x1) * doubleArea;
			sumY += (y0 + y1) * doubleArea;
		}

		float cu = sumX / (6.0 * area);
		float cv = sumY / (6.0 * area);

		SVector3 cucv = SVector3(1, cu, cv);
		SVector3 xuv = SVector3(v0.X, u.X, v.X);
		SVector3 yuv = SVector3(v0.Y, u.Y, v.Y);
		SVector3 zuv = SVector3(v0.Z, u.Z, v.Z);
		float x = xuv.Dot(cucv);
		float y = yuv.Dot(cucv);
		float z = zuv.Dot(cucv);
		return SVector3(x, y, z);
	}*/

	void NavMesh::read(uintptr_t p_data, uint32_t p_filesize)
	{
		uintptr_t s_startPointer = p_data;
		uintptr_t s_endPointer{};

		m_hdr = (Binary::Header*) p_data;
		p_data += sizeof(Binary::Header);

		m_sectHdr = (Binary::SectionHeader*) p_data;
		p_data += sizeof(Binary::SectionHeader);

		m_setHdr = (Binary::NavSetHeader*) p_data;
		p_data += sizeof(Binary::NavSetHeader);

		m_graphHdr = (Binary::NavGraphHeader*) p_data;
		p_data += sizeof(Binary::NavGraphHeader);

		FixAreaPointers(p_data - sizeof(Binary::NavGraphHeader), m_graphHdr->m_areaBytes);

		s_endPointer = p_data + m_graphHdr->m_areaBytes;
		while (p_data < s_endPointer) {
			Area s_area{};
			s_area.m_area = (Binary::Area*) p_data;
			p_data += sizeof(Binary::Area);

			for (uint32_t i = 0; i < s_area.m_area->m_flags.GetNumEdges(); ++i) {
				s_area.m_edges.push_back((Binary::Edge*) p_data);
				p_data += sizeof(Binary::Edge);
			}

			m_areas.push_back(s_area);
		}
		m_kdTreeData = (Binary::KDTreeData*) p_data;
		p_data += sizeof(Binary::KDTreeData);

		m_rootKDNode = (Binary::KDNode*) p_data;

		// This is just for filesize sanity checking
		s_endPointer = p_data + m_kdTreeData->m_size;
		while (p_data < s_endPointer) {
			Binary::KDNode* s_KDNode = (Binary::KDNode*) p_data;
			Binary::KDLeaf* s_KDLeaf = (Binary::KDLeaf*) p_data;

			if (s_KDNode->IsLeaf())
				p_data += sizeof(Binary::KDLeaf);
			else
				p_data += sizeof(Binary::KDNode);
		}

		// Sanity check
		if ((p_data - s_startPointer) != p_filesize) {
			printf("[WARNING] What we read does not equal filesize!\n");
		}
	}
}// namespace NavPower
