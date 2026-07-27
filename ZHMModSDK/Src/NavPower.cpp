#include "NavPower.h"

uint32_t RangeCheck(uint32_t val, uint32_t min, uint32_t max) {
    if (val > max) return max;
    if (val < min) return min;
    return val;
}

namespace NavPower {
    void FixAreaPointers(uintptr_t data, size_t areaBytes) {
        uintptr_t navGraphStart = data;
        uintptr_t curIndex = data + sizeof(Binary::NavGraphHeader);
        size_t areaEndPtr = curIndex + areaBytes;

        while (curIndex != areaEndPtr) {
            Binary::Area* curArea = reinterpret_cast<Binary::Area*>(curIndex);
            curIndex += sizeof(Binary::Area);
            for (uint32_t i = 0; i < curArea->m_flags.GetNumEdges(); i++) {
                Binary::Edge* curEdge = (Binary::Edge*) curIndex;
                curIndex += sizeof(Binary::Edge);

                Binary::Area* adjArea = curEdge->m_pAdjArea;
                if (adjArea != NULL)
                    curEdge->m_pAdjArea = (Binary::Area*) (navGraphStart + (char*) adjArea);
            }
        }
    }

    SVector3 Area::CalculateCentroid() {
        SVector3 normal = CalculateNormal();
        SVector3 v0 = m_edges.at(0)->m_pos;
        SVector3 v1 = m_edges.at(1)->m_pos;

        SVector3 u = (v1 - v0).GetUnitVec();
        SVector3 v = u.Cross(normal).GetUnitVec();

        std::vector<SVector3> mappedPoints;
        for (Binary::Edge* edge : m_edges) {
            SVector3 relativePos = edge->m_pos - v0;
            float uCoord = relativePos.Dot(u);
            float vCoord = relativePos.Dot(v);
            SVector3 uvv = SVector3(uCoord, vCoord, 0.0);
            mappedPoints.push_back(uvv);
        }
        float sum = 0;
        for (int i = 0; i < mappedPoints.size(); i++) {
            int nextI = (i + 1) % mappedPoints.size();
            sum += mappedPoints[i].x * mappedPoints[nextI].y - mappedPoints[nextI].x * mappedPoints[i].y;
        }
        float area = sum / 2;
        if (area < 0) {
            area *= -1;
        }

        float sumX = 0;
        float sumY = 0;
        for (int i = 0; i < mappedPoints.size(); i++) {
            int nextI = (i + 1) % mappedPoints.size();
            float x0 = mappedPoints[i].x;
            float x1 = mappedPoints[nextI].x;
            float y0 = mappedPoints[i].y;
            float y1 = mappedPoints[nextI].y;

            float doubleArea = (x0 * y1) - (x1 * y0);
            sumX += (x0 + x1) * doubleArea;
            sumY += (y0 + y1) * doubleArea;
        }

        float cu = sumX / (6.0 * area);
        float cv = sumY / (6.0 * area);

        SVector3 cucv = SVector3(1, cu, cv);
        SVector3 xuv = SVector3(v0.x, u.x, v.x);
        SVector3 yuv = SVector3(v0.y, u.y, v.y);
        SVector3 zuv = SVector3(v0.z, u.z, v.z);
        float x = xuv.Dot(cucv);
        float y = yuv.Dot(cucv);
        float z = zuv.Dot(cucv);
        return SVector3(x, y, z);
    }

    void NavGraph::read(uintptr_t& p_data)
    {
        uintptr_t s_startPointer = p_data;
        uintptr_t s_endPointer{};

        m_hdr = (Binary::NavGraphHeader*)p_data;
        p_data += sizeof(Binary::NavGraphHeader);

        FixAreaPointers(p_data - sizeof(Binary::NavGraphHeader), m_hdr->m_areaBytes);

        s_endPointer = p_data + m_hdr->m_areaBytes;
        while (p_data < s_endPointer) {
            Area s_area{};
            s_area.m_area = (Binary::Area*)p_data;
            p_data += sizeof(Binary::Area);

            for (uint32_t i = 0; i < s_area.m_area->m_flags.GetNumEdges(); ++i) {
                s_area.m_edges.push_back((Binary::Edge*)p_data);
                p_data += sizeof(Binary::Edge);
            }

            m_areas.push_back(s_area);
        }

        m_kdTreeData = (Binary::KDTreeData*)p_data;
        p_data += sizeof(Binary::KDTreeData);

        m_rootKDNode = (Binary::KDNode*)p_data;

        s_endPointer = p_data + m_kdTreeData->m_size;
        while (p_data < s_endPointer) {
            Binary::KDNode* s_KDNode = (Binary::KDNode*)p_data;
            Binary::KDLeaf* s_KDLeaf = (Binary::KDLeaf*)p_data;

            if (s_KDNode->IsLeaf())
                p_data += sizeof(Binary::KDLeaf);
            else
                p_data += sizeof(Binary::KDNode);
        }

        if ((p_data - s_startPointer) != m_hdr->m_totalBytes) {
            printf("[WARNING] NavGraph - What we read does not match the total bytes!\n");
        }
    }

    void Section::read(uintptr_t& p_data) {
        m_hdr = (Binary::SectionHeader*)p_data;
        p_data += sizeof(Binary::SectionHeader);

        if (m_hdr->m_size == 0) return;

        // Sectionheader->m_size excludes the header.
        uintptr_t s_startPointer = p_data;
        uintptr_t s_endPointer{};

        m_setHdr = (Binary::NavSetHeader*)p_data;
        p_data += sizeof(Binary::NavSetHeader);

        for (uint32_t i = 0; i < m_setHdr->m_numGraphs; ++i) {
            m_aNavGraphs.push_back(NavGraph(p_data));
        }

        if ((p_data - s_startPointer) != m_hdr->m_size) {
            printf("[WARNING] Section - What we read does not match the section size!\n");
        }
    }

    void NavMesh::read(uintptr_t p_data, uint32_t p_filesize) {
        uintptr_t s_startPointer = p_data;

        m_hdr = (Binary::Header*)p_data;
        p_data += sizeof(Binary::Header);

        // Read Sections
        while ((p_data - s_startPointer) != p_filesize) {
            m_aSections.push_back(Section(p_data));
        }
    }
} // namespace NavPower