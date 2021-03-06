/* TableData.h

   Copyright (C) 2015, 2016 Marc Postema (mpostema09 -at- gmail.com)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   Or, point your browser to http://www.gnu.org/copyleft/gpl.html
*/
#ifndef MPEGTS_TABLE_DATA_H_INCLUDE
#define MPEGTS_TABLE_DATA_H_INCLUDE MPEGTS_TABLE_DATA_H_INCLUDE

#include <string>

namespace mpegts {

	class TableData {
		public:
			#define CRC(data, sectionLength) \
				(data[sectionLength - 4 + 8] << 24) |     \
				(data[sectionLength - 4 + 8 + 1] << 16) | \
				(data[sectionLength - 4 + 8 + 2] <<  8) | \
				 data[sectionLength - 4 + 8 + 3]

			#define PAT_TABLE_ID           0x00
			#define CAT_TABLE_ID           0x01
			#define PMT_TABLE_ID           0x02
			#define SDT_TABLE_ID           0x42
			#define EIT1_TABLE_ID          0x4E
			#define EIT2_TABLE_ID          0x4F
			#define ECM0_TABLE_ID          0x80
			#define ECM1_TABLE_ID          0x81
			#define EMM1_TABLE_ID          0x82
			#define EMM2_TABLE_ID          0x83
			#define EMM3_TABLE_ID          0x84

			// ================================================================
			// -- Constructors and destructor ---------------------------------
			// ================================================================

			TableData();

			virtual ~TableData();

			// ================================================================
			//  -- Static member functions ------------------------------------
			// ================================================================

		public:

			static uint32_t calculateCRC32(const unsigned char *data, int len);

			// ================================================================
			//  -- Other member functions -------------------------------------
			// ================================================================

		public:

			///
			const char* getTableTXT() const;

			/// Collect Table data for tableID
			void collectData(int streamID, int tableID, const unsigned char *data);

			/// Get the collected Table Data
			const unsigned char *getData() const {
				return reinterpret_cast<const unsigned char *>(_data.c_str());
			}

			/// Get the collected Table Data
			void getData(std::string &data) const {
				data = _data;
			}

			/// Get the current size of the Table Packet
			int getDataSize() const {
				return _data.size();
			}

			/// Set if Table has been collected or not
			void setCollected(bool collected) {
				_collected = collected;
				if (!collected) {
					_cc  = -1;
					_pid = -1;
					_data.clear();
				}
			}

			/// Check if Table is collected
			bool isCollected() const {
				return _collected;
			}

		protected:

			/// Add Table data that was collected
			bool addData(int tableID, const unsigned char *data, int length, int pid, int cc);

			// ================================================================
			//  -- Data members -----------------------------------------------
			// ================================================================

		private:

			int _tableID;
			std::string _data;
			int _cc;
			int _pid;
			bool _collected;
	};

} // namespace mpegts

#endif // MPEGTS_TABLE_DATA_H_INCLUDE
