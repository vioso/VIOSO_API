// VIOSO API libpng wrapper
// http://bitbucket.org/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2024
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#include "png.h"
#include <exception>
#include <vector>
#include <fstream>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <type_traits>

#ifdef CPNG_LINK_DEP_LIB
#pragma comment( lib, "libpng16_static" )
#pragma comment( lib, "zlibstatic" )
#endif

template <int depth_>
class bit_nibble_iterator {
    static_assert(depth_ == 1 || depth_ == 2 || depth_ == 4, "Depth must be 1, 2, or 4 bits");
    static constexpr int baseShift = (depth_ == 4) ? 1 : ((depth_ == 2) ? 2 : 3);
    static constexpr uint8_t baseMask = (depth_ == 4) ? 0b00000001 : ((depth_ == 2) ? 0b00000011 : 0b00000111);
    static constexpr uint8_t valMask = (depth_ == 4) ? 0b00001111 : ((depth_ == 2) ? 0b00000011 : 0b00000001);

    uintptr_t data_;

    inline static const uint8_t* baseAddr(uintptr_t d) {
        return reinterpret_cast<const uint8_t*>(d >> baseShift);
    }

    inline static uintptr_t shift(uintptr_t d) {
        return (8 - depth_) - ((d & baseMask) * depth_);
    }

public:
    using iterator_category = std::random_access_iterator_tag;
//    using value_type = std::bitset<depth_>;
    using value_type = uint8_t;
    using difference_type = std::ptrdiff_t;
    using pointer = const uint8_t*;
    using reference = uint8_t;

    bit_nibble_iterator(const uint8_t* data, size_t index = 0)
        : data_((reinterpret_cast<uintptr_t>(data) << baseShift) + index) {}

    value_type operator*() const {
        return (*baseAddr(data_) >> shift(data_)) & valMask;
    }

    bit_nibble_iterator& operator++() {
        ++data_;
        return *this;
    }

    bit_nibble_iterator operator++(int) {
        bit_nibble_iterator temp = *this;
        ++(*this);
        return temp;
    }

    bool operator==(const bit_nibble_iterator& other) const {
        return data_ == other.data_;
    }

    bool operator!=(const bit_nibble_iterator& other) const {
        return !(*this == other);
    }

    bit_nibble_iterator& operator+=(difference_type off) {
        data_ += off;
        return *this;
    }

    bit_nibble_iterator& operator-=(difference_type off) {
        data_ -= off;
        return *this;
    }

    bit_nibble_iterator operator+(difference_type off) const {
        bit_nibble_iterator ret = *this;
        ret.data_ += off;
        return ret;
    }

    bit_nibble_iterator operator-(difference_type off) const {
        bit_nibble_iterator ret = *this;
        ret.data_ -= off;
        return ret;
    }

    difference_type operator-(const bit_nibble_iterator& other) const {
        return data_ - other.data_;
    }

    value_type operator[](difference_type off) const {
        return *(*this + off);
    }

    bit_nibble_iterator& set(value_type v) {
        auto s = shift(data_);
        auto* vv = reinterpret_cast<uint8_t*>(data_ >> baseShift);
        *vv &= ~(valMask << s);
        *vv |= (v.to_ulong() << s);
        return *this;
    }

    //bit_nibble_iterator& set(uint8_t v) {
    //    auto s = shift(data_);
    //    auto* vv = reinterpret_cast<uint8_t*>(data_ >> baseShift);
    //    *vv &= ~( valMask << s );
    //    *vv |= ( v & valMask ) << s;
    //    return *this;
    //}
};

class CPng
{
public:
    png_uint_32 m_width;
    png_uint_32 m_height;
    int m_bitDepth;
    int m_colorType;
    std::vector<uint8_t> m_imageData;
    std::vector<png_color> m_paletteData;
private:
    png_structp m_pPngR;
    png_structp m_pPngW;
    png_infop m_pInfo;
    int m_interlace;
    int m_compression;
    int m_filter;
    std::vector<png_bytep> m_rowPointers;

    constexpr static int min_value(int a, int b) {
        return (a < b) ? a : b;
    }

    constexpr static int max_value(int a, int b) {
        return (a > b) ? a : b;
    }

    constexpr static int clamp_value(int min, int max, int v) {
        return (v < min) ? min : ( v > max ? max : v );
    }

    constexpr static uint8_t clampU8( int v ) {
        return (v < 0) ? 0 : ( v > 255 ? 255 : uint8_t(v) );
    }

    static void srdfn( png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead )
    {
        auto io = (std::istream*)png_get_io_ptr( png_ptr );
        if( !io || io->bad() )
            throw std::exception( "png read parameter bad" );
        if( io->read( (char*)outBytes, byteCountToRead ).fail() )
            throw std::exception( "png read out of bound" );
    }

    static void swtfn( png_structp png_ptr, png_bytep inBytes, png_size_t byteCountToWrite )
    {
        auto io = (std::ostream*)png_get_io_ptr( png_ptr );
        if( !io || io->bad() )
            throw std::exception( "png write parameter bad" );
        if( io->write( (char*)inBytes, byteCountToWrite ).fail() )
            throw std::exception( "png write out of bound" );
    }

    class imemstream : public std::istream {
        class MemoryBuffer : public std::streambuf {
        public:
            MemoryBuffer(const char* base, size_t size) {
                setg(const_cast<char*>(base), const_cast<char*>(base), const_cast<char*>(base) + size);
            }
        } buff;
    public:
        imemstream(const char* base, size_t size)
            : std::istream(&buff), buff(base, size) {}
    };

    class omemstream : public std::ostream {
        class MemoryBuffer : public std::streambuf {
        public:
            MemoryBuffer(char* base, size_t size) {
                setp(base, base, base + size);
            }
        } buff;
    public:
        omemstream(char* base, size_t size)
            : std::ostream(&buff), buff(base, size) {}
    };

public:
    // read from memory
    CPng()
        : m_width{}
        , m_height{}
        , m_bitDepth{}
        , m_colorType{}
        , m_pPngR{}
        , m_pPngW{}
        , m_pInfo{}
        , m_interlace{}
        , m_compression{}
        , m_filter{}
    {}

    // read from stream
    CPng( std::istream& is ) : CPng() {
        if( is.bad() )
            throw std::exception( "file not found" );
 
        uint8_t sig[8];
        is.read( (char*)sig, 8 );

        if( !png_check_sig( (png_byte const*)sig, 8 ) )
            throw std::exception( "unknown png signature" );
        m_pPngR = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL,
                                         NULL );
        if( !m_pPngR )
            throw std::exception( "failed to create png read struct" );
        m_pInfo = png_create_info_struct( m_pPngR );
        if( !m_pInfo )
            throw std::exception( "failed to create png info struct" );

        if( setjmp( png_jmpbuf( m_pPngR ) ) )
            throw std::exception( "Error: libpng encountered an error " );

        png_set_read_fn( m_pPngR, &is, &srdfn );

        png_set_sig_bytes( m_pPngR, 8 );
        png_read_info( m_pPngR, m_pInfo );
        png_get_IHDR(
            m_pPngR, m_pInfo,
            &m_width,
            &m_height,
            &m_bitDepth,
            &m_colorType,
            &m_interlace,
            &m_compression,
            &m_filter
        );
        if( PNG_INTERLACE_NONE != m_interlace )
            throw std::exception( "interlaced image read not implemented" );
        if( PNG_FILTER_TYPE_BASE != m_interlace )
            throw std::exception( "filtered image read not implemented" );

        size_t ch;
        if( m_colorType & PNG_COLOR_MASK_COLOR )
            ch = 3;
        else
            ch = 1;
        if( m_colorType & PNG_COLOR_MASK_ALPHA )
            ch++;

        if(m_colorType & PNG_COLOR_MASK_PALETTE) {
            png_colorp palette;
            int nPalette;
            png_get_PLTE(m_pPngR, m_pInfo, &palette, &nPalette);
            m_paletteData.reserve(nPalette);
            m_paletteData.insert( m_paletteData.begin(), palette, palette + nPalette );
        }

        const size_t lDst = ( size_t( m_width ) * ch * m_bitDepth + 7 ) / 8;
        const size_t s = lDst * m_height;
        m_imageData.resize( s );

        for( auto px = m_imageData.data(), pxE = px + s; px != pxE; px += lDst )
        {
            png_read_row( m_pPngR, px, nullptr );
        }

    }

    // read from file
    CPng(std::filesystem::path const& path) : CPng( (std::istream&)std::ifstream(path, std::ios::binary | std::ios::in ) ) {}
        
    // read from memory
    CPng(char* data, size_t size) : CPng( (std::istream&)imemstream(data, size) ) {}

    // create a container
    CPng(png_uint_32 width, png_uint_32 height, int colorType = PNG_COLOR_TYPE_RGB, int depth = 8 ) 
        : m_width(width)
        , m_height(height)
        , m_colorType(colorType)
        , m_bitDepth( depth )
        , m_pPngR{}
        , m_pPngW{}
        , m_pInfo{}
        , m_interlace{}
        , m_compression{}
        , m_filter{}
    {
        switch(m_bitDepth) {
            case 1:
            case 2:
            case 4:
            case 8:
            case 16:
                break;
            default:
                throw std::invalid_argument("Wrong bit depth");
        }
        switch(m_colorType) {
            case PNG_COLOR_TYPE_GRAY:
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                break;
            case PNG_COLOR_TYPE_RGB:
            case PNG_COLOR_TYPE_RGB_ALPHA:
                if( m_bitDepth < 8 )
                    throw std::invalid_argument("Bit depth too low for PNG_COLOR_TYPE_RGB or PNG_COLOR_TYPE_RGB_ALPHA");
                break;
            case PNG_COLOR_TYPE_PALETTE:
                if( m_bitDepth > 8 )
                    throw std::invalid_argument("Bit depth too high for PNG_COLOR_TYPE_PALETTE");
                break;
            default:
                throw std::invalid_argument("Wrong color type");
        }

        //if( colorType == PNG_COLOR_TYPE_PALETTE && 
        m_pPngW = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        m_pInfo = png_create_info_struct(m_pPngW);

        if( setjmp( png_jmpbuf( m_pPngW ) ) )
            throw std::exception( "Error: libpng encountered an error " );
    }

	~CPng()
	{
        if( m_pPngR )
            png_destroy_read_struct( &m_pPngR, &m_pInfo, nullptr );
        if( m_pPngW )
            png_destroy_write_struct( &m_pPngW, &m_pInfo );
    }

    template< typename TI >
    bool setImgData( TI in, TI inEnd, int nChannels = 3, int scaleNum = 1, int scaleDenom = 1, int nR = 0, int nG = 1, int nB = 2, int nA = 3 ) {
        using value_type = typename std::iterator_traits<TI>::value_type;
        if(PNG_COLOR_TYPE_PALETTE == m_colorType) {
            // create map of levels, recording locations of this RGB uplift, a final color and a palette index
            typedef struct Bin{
                union {
                    uint32_t data;
                    struct alignas(1) {
                        uint8_t r;
                        uint8_t g;
                        uint8_t b;
                        uint8_t a;
					} rgb8u;
				};
                Bin() : data(0) {};
                Bin(png_color const& col) : rgb8u{ clampU8(col.red), clampU8(col.green), clampU8(col.blue), 0 } {}
                Bin(int r, int g, int b) : rgb8u{ clampU8(r), clampU8(g), clampU8(b), 0 } {}
                inline explicit Bin( uint32_t d ) : data(d) {}
                static inline Bin e() { return Bin(-2); } // essential color reference
                static inline Bin n() { return Bin(-1); } // needed color reference

                struct Hasher {
                    inline std::size_t operator()(Bin const& b) const { return std::hash<uint32_t>{}(b.data); }
                };
                inline bool operator==(Bin const& other) const { return data == other.data; }
                inline bool operator!=(Bin const& other) const { return data != other.data; }
				inline bool operator<(Bin const& other) const { return rgb8u.r + rgb8u.g + rgb8u.b < other.rgb8u.r + other.rgb8u.g + other.rgb8u.b; }
                inline operator png_color() const {
                    return png_color{ uint8_t(data), uint8_t(data >> 8), uint8_t(data >> 16) };
                }

                inline Bin& average( Bin const& rhs ) { 
					rgb8u.r += rhs.rgb8u.r; rgb8u.r /= 2;
					rgb8u.g += rhs.rgb8u.g; rgb8u.g /= 2;
					rgb8u.b += rhs.rgb8u.b; rgb8u.b /= 2;
                    return *this;
                }
				/// @override
                inline static Bin average(Bin const& lhs, Bin const& rhs) { 
                    return Bin(lhs).average(rhs);
                }

                inline Bin& pivotedAverage( Bin const& rhs, int c1, int c2 ) { 
					uint32_t c = c1 + c2;
					uint32_t h = c / 2; // round not floor!
                    rgb8u.r = uint8_t( ( c1 * rgb8u.r + c2 * rhs.rgb8u.r + h ) / c );
					rgb8u.g = uint8_t( ( c1 * rgb8u.g + c2 * rhs.rgb8u.g + h ) / c );
					rgb8u.b = uint8_t( ( c1 * rgb8u.b + c2 * rhs.rgb8u.b + h ) / c );
                    return *this;
                }
				/// @override
				inline static Bin pivotedAverage(Bin const& lhs, Bin const& rhs, int c1, int c2) {
					return Bin(lhs).pivotedAverage(rhs, c1, c2);
				}

                inline bool hasReference() const { return 0 == ( data & 0x80000000 ); }

                inline uint8_t r() const { return uint8_t(data & 0xFF); }
                inline uint8_t g() const { return uint8_t((data & 0xFF00) >> 8); }
                inline uint8_t b() const { return uint8_t((data & 0xFF0000) >> 16); }

                inline uint32_t dist2(Bin const& other) const { // the squared distance, used to find close colors
                    int32_t r_ = r() - other.r(); r_*= r_;
                    int32_t g_ = g() - other.r(); g_*= g_;
                    int32_t b_ = b() - other.r(); b_*= b_;

                    return r_ + g_ + b_;
                }

            } Bin;

            std::unordered_map<Bin, std::pair<Bin, int>, typename Bin::Hasher> levels; // mapping a color to a target color and a palette index
            // fill map with data
            if constexpr( std::is_floating_point_v<value_type> ) {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels)
                    if(auto [it, bNew] = levels.emplace(Bin(int(pB[nR] * scaleNum / scaleDenom), int(pB[nG] * scaleNum / scaleDenom), int(pB[nB] * scaleNum / scaleDenom)), std::pair<Bin, int>{ Bin::n(), 1 }); !bNew)
                        it->second.second++;

            } else {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels)
					if(auto [it, bNew] = levels.emplace(Bin(int(pB[nR]) * scaleNum / scaleDenom, int(pB[nG]) * scaleNum / scaleDenom, int(pB[nB]) * scaleNum / scaleDenom), std::pair<Bin, int>{ Bin::n(), 1 }); !bNew)
						it->second.second++;
            }

            const int maxLevels = size_t(1) << m_bitDepth;
            if(levels.size() > maxLevels) {
                ////////////
                // gather essential colors

                // build a by count sorted list of colors
                std::vector<std::pair<Bin, int >> sorted(levels.size());
                auto itS = sorted.begin();
                int all = 0;
                for(auto const& l : levels) {
                    itS->first = l.first;
                    itS->second = l.second.second;
                    all += l.second.second;
                    itS++;
                }
                std::sort(sorted.begin(), sorted.end(), [](auto const& a, auto const& b) { return a.second > b.second; });

                // calculate max colors and levels
                const int maxColors = all * 4 / 10;

                // set all black essential
                int accu = levels[Bin(0, 0, 0)].second;
                int essential = 1;
                levels[Bin(0, 0, 0)].first = Bin::e(); // mark as essential

				// find brightest color and set as essential
                if(0) { // has rather negative effect, as we lose a step and some darker color
                    Bin max(0, 0, 0);
                    ptrdiff_t iMax = INT_MAX;
                    for(auto const& l : sorted) {
                        if(max < l.first) {
                            max = l.first;
							iMax = &l - &sorted[0];
                        }
                    }
                    if(iMax != INT_MAX) {
                        levels[ sorted[iMax].first ].first = Bin::e(); // mark as essential
                        essential++;
					}
                }

                // find most frequent essential colors
                for( auto const& l : sorted ) {
                    if( accu >= maxColors )
                        break;
                    if( essential >= maxLevels )
                        break;

                    if( Bin(0, 0, 0) == l.first ) // skip black
                        continue;

                    accu += l.second;
                    essential++;
                    levels[l.first].first = Bin::e(); // mark as essential
                }

                // merge levels, if more than 2^depth
                int nLevels = (int)levels.size(); // conversion to int is fine, there are max 2^24 colors
                while(nLevels > maxLevels)
                {
                    auto m1 = levels.end();
                    auto m2 = levels.end();

                    // find closest pair
                    uint32_t sMin = UINT_MAX;

                    for(auto a = levels.begin(); a != levels.end(); a++)
                    {
                        if(a->second.first.hasReference())
                            continue;

                        for(auto b = a; ++b != levels.end(); )
                        {
                            if(b->second.first.hasReference())
                                continue;

							if(Bin::e() == a->second.first && Bin::e() == b->second.first) // skip essential
                                continue;

                            auto s = a->first.dist2(b->first);
                            if( s < sMin || ( s <= sMin && m1->second.second + m2->second.second > a->second.second + b->second.second ) ) { // lower distance or same distance and less colors
                                m1 = a;
								m2 = b;
                                sMin = s;
                            }
                        }
                    }

#ifdef _DEBUG
                    // debug count needed before
                    int n1 = 0;
                    for(auto& l : levels) {
                        if( !l.second.first.hasReference() )
                            n1++;
                    }
                    if(n1 != nLevels)
                        int u = 0;

#endif // def _DEBUG
                    // merge pair, no need to check, as there is always a valid pair
                    // tree iterators are no L-values, so we need to create a new Bin and delete old then insert new
                    auto iIt = levels.end();
                    if( m1->second.first == Bin::e() ) { //m1 essential
                        iIt = m1;
                    } else if( m2->second.first == Bin::e() ) { //m2 essential
                        iIt = m2;
                    } else {
                        auto avg = Bin::pivotedAverage( m1->first, m2->first, m1->second.second, m2->second.second );
                        auto [oiIt, bNew] = levels.emplace(std::move(avg), std::pair<Bin, int>{ Bin::n(), 0 });
                        iIt = oiIt;
                        if( !bNew ) {
                            if( iIt->second.first.hasReference() ) {
                                iIt->second.first = Bin::n(); // we revive that color and mark as needed
                            } else {
                                if( m1->first != iIt->first && m2->first != iIt->first ) {
                                    // here is a very spacial case, where a merge hit an existing needed color, which was not m1 or m2
                                    // this decreases the number of valid levels another time
                                    nLevels--;
                                }
                            }
                        }
                    }
                    nLevels--;

                    // reference the original colors to the new color
                    if(m1->first != iIt->first) { // if m1 is not same as new, we set target to the merged color
                        m1->second.first = iIt->first; // set reference color
                        iIt->second.second += m1->second.second; // merge count
						m1->second.second = 0; // set count to 0
                    }
                    if(m2->first != iIt->first) {
                        m2->second.first = iIt->first; // set reference color
						iIt->second.second += m2->second.second; // merge count
						m2->second.second = 0; // set count to 0
                    }

#ifdef _DEBUG
                    // debug count needed after
                    int n2 = 0;
                    for(auto& l : levels) {
                        if( !l.second.first.hasReference() )
                            n2++;
                    }
                    if(n2 != nLevels)
                        int u = 0;
#endif // def _DEBUG
                }
            }
            // create palette indices
            // iterating the entries and enumerate all, that hasn't been merged down, and save palette data
            int pal = 0;
            m_paletteData.resize( size_t(1) << m_bitDepth);
            for(auto& l : levels) {
                if(!l.second.first.hasReference() ) {
                    m_paletteData[pal] = l.first;
                    l.second.second = pal++;
                }
            }

            struct Weights {
                uint8_t n;
                uint8_t n1;
            } const weights[] = {
                { 1, 1 }, // { total, part of col1 }
                { 5, 4 },
                { 4, 3 },
                { 3, 2 },
                { 5, 3 },
                { 2, 1 },
                { 5, 2 },
                { 3, 1 },
                { 4, 1 },
                { 5, 1 },
                { 1, 0 },
            };
            // set palette indices for merged colors
            struct alignas(1) DitherInfo {
                uint8_t col1;
                uint8_t col2;
                uint8_t w; // weight
            };

			std::vector<DitherInfo> ditherData;
			ditherData.reserve(levels.size());
			for(auto& l : levels) {
				if( l.second.first.hasReference()) {
                    // for a dithered color, we need to find the next two colors
                    // first is the indexed of the color itself
                    auto lIt1 = levels.find(l.second.first);
					while(lIt1->second.first.hasReference())
						lIt1 = levels.find(lIt1->second.first);
                    // now we need to find the other next color
                    auto r = l.first.r();
                    auto g = l.first.g();
                    auto b = l.first.b();
                    auto r1 = lIt1->first.r();
                    auto g1 = lIt1->first.g();
                    auto b1 = lIt1->first.b();

                    auto lIt2 = levels.end();
					uint32_t sMin = UINT_MAX;
					for(auto it = levels.begin(); it != levels.end(); it++) {
						if(it->first == lIt1->first)
							continue;
						if(it->second.first.hasReference()) // skip referenced
							continue;
						auto s = l.first.dist2(it->first);
                        if( s <= sMin ) { // lower distance or same distance and less colors
                            // each channel must be same or other direction 
                            auto r2 = it->first.r();
                            auto g2 = it->first.g();
                            auto b2 = it->first.b();
                            if( ( r1 <= r && r <= r2 && 
                                  g1 <= g && g <= g2 && 
                                  b1 <= b && b <= b2 ) ||
                                ( r2 <= r && r <= r1 &&
                                  g2 <= g && g <= g1 &&
                                  b2 <= b && b <= b1 ) ) {
                                lIt2 = it;
                                sMin = s;
                            }
                        }
					}

                    if( lIt2 != levels.end() && sMin < 22 ) {
                    // find weights, we try to find the best match
                        uint8_t best = 0;
                        float sMin = FLT_MAX;
                        for(auto const& w : weights) {
							auto n2 = w.n - w.n1;
                            auto dr = float( uint32_t(lIt1->first.r()) * w.n1 + uint32_t(lIt2->first.r()) * n2 ) / w.n - l.first.r();
                            auto dg = float( uint32_t(lIt1->first.g()) * w.n1 + uint32_t(lIt2->first.g()) * n2 ) / w.n - l.first.g();
                            auto db = float( uint32_t(lIt1->first.b()) * w.n1 + uint32_t(lIt2->first.b()) * n2 ) / w.n - l.first.b();

                            auto s = dr * dr + dg * dg + db * db;
                            if(s < sMin) {
                                best = int(&w - &weights[0]);
                                sMin = s;
                            }
                        }
                        // add
                        ditherData.emplace_back( DitherInfo{ uint8_t( lIt1->second.second ), uint8_t( lIt2->second.second ), best } );
                    }
                    else {
                        ditherData.emplace_back( DitherInfo{ uint8_t( lIt1->second.second ), 0 , 0 } ); // just the color itself
                    }
				} else {
					ditherData.emplace_back( DitherInfo{ uint8_t( l.second.second ), 0 ,0 } ); // just the color itself
				}
			}

            // update color index to ditherData index
			pal = 0;
            for(auto& l : levels) {
				l.second.second = pal++;
            }

            // fill raw data
            srand(42);
            std::vector<uint8_t> rawIndex(m_width* m_height, 0);
            size_t i = 0;
            if constexpr( std::is_floating_point_v<value_type> ) {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels, i++ )
                {
					Bin cc(int(pB[nR] * scaleNum / scaleDenom), int(pB[nG] * scaleNum / scaleDenom), int(pB[nB] * scaleNum / scaleDenom));
					auto const& dd = ditherData[levels[cc].second];
					if( rand() * weights[dd.w].n / ( RAND_MAX + 1 ) < weights[dd.w].n1 )
						rawIndex[i] = dd.col1;
					else
						rawIndex[i] = dd.col2;
                }
            } else {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels, i++)
                {
                    Bin cc(int(pB[nR]) * scaleNum / scaleDenom, int(pB[nG]) * scaleNum / scaleDenom, int(pB[nB]) * scaleNum / scaleDenom);
                    auto const& dd = ditherData[levels[cc].second];
                    if( rand() * weights[dd.w].n / ( RAND_MAX + 1 ) < weights[dd.w].n1 )
                        rawIndex[i] = dd.col1;
                    else
                        rawIndex[i] = dd.col2;
                }
            }

            m_rowPointers.resize(m_height);
            if(m_bitDepth == 8) {
                m_imageData = std::move(rawIndex);
                for( int row = 0; row != m_height; row++ )
                    m_rowPointers[row] = m_imageData.data() + row * m_width;
            } else {
                // fold into (smaller) depth
                size_t lnSz = ( size_t(m_width) * m_bitDepth + 7 ) / 8;
                m_imageData.resize(lnSz * m_height + 1 );
                auto d = m_imageData.begin();
                auto padd = lnSz - size_t(m_width) * m_bitDepth / 8;
                size_t row = 0;
                for(auto ln = rawIndex.begin(); ln != rawIndex.end(); ln+= m_width, d+= padd, row++ ) {
                    int shift = 8;
                    *d = 0;
                    m_rowPointers[row] = &(*d);
                    for(auto i = ln, lnE = ln + m_width; i != lnE; i++) {
                        shift -= m_bitDepth;
                        *d += *i  << shift;
                        if(shift == 0) {
                            shift = 8;
                            *(++d) = 0;
                        }
                    }
                }
            }
        } else {
            m_paletteData.clear();
            int ch;
            if( m_colorType & PNG_COLOR_MASK_COLOR )
                ch = 3;
            else
                ch = 1;
            if( m_colorType & PNG_COLOR_MASK_ALPHA )
                ch++;

            if(nChannels != ch)
                return false; // must be same number of channels

            size_t lnSz = size_t(int64_t(m_width) * ch * m_bitDepth + 7) / 8;
            m_imageData.resize( lnSz * m_height + 1 );
            auto it = in;
            int in2out[] = { nR, nG, nB, nA };
            if(8 == m_bitDepth) {
                if constexpr( std::is_floating_point_v<value_type> ) {
                    for(auto dIt = m_imageData.begin(); dIt!= m_imageData.end(); it+= nChannels ) {
                        for(int c = 0; c != nChannels; c++) {
                            *(dIt++) = uint8_t( clamp_value(0, 255, int(it[in2out[c]] * scaleNum / scaleDenom)));
                        }
                    }
                } else {
                    for(auto dIt = m_imageData.begin(); dIt!= m_imageData.end(); it+= nChannels ) {
                        for(int c = 0; c != nChannels; c++) {
                            *(dIt++) = uint8_t( clamp_value( 0, 255, png_uint_16( it[in2out[c]]) * scaleNum / scaleDenom ) );
                        }
                    }
                }
            } if(16 == m_bitDepth) {
                if constexpr( std::is_floating_point_v<value_type> ) {
                    for(auto dIt = (png_uint_16*)m_imageData.data(), dItE = (png_uint_16*)(m_imageData.data() + m_imageData.size()); dIt!= dItE; it+= nChannels ) {
                        for(int c = 0; c != nChannels; c++) {
                            *(dIt++) = png_uint_16( clamp_value( 0, UINT16_MAX, int( it[in2out[c]] * scaleNum / scaleDenom ) ) );
                        }
                    }
                } else {
                    for(auto dIt = (png_uint_16*)m_imageData.data(), dItE = (png_uint_16*)(m_imageData.data() + m_imageData.size()); dIt!= dItE; it+= nChannels ) {
                        for(int c = 0; c != nChannels; c++) {
                            *(dIt++) = png_uint_16( clamp_value( 0, UINT16_MAX, png_uint_32( it[in2out[c]] ) * scaleNum / scaleDenom ) );
                        }
                    }
                }
            } else {
                if(ch != 1)
                    return false;
                auto d = m_imageData.begin();
                const auto padd = lnSz - size_t(m_width) * m_bitDepth / 8;
                const uint8_t mask = (1 << m_bitDepth) -1;

                for(auto ln = in, lnE = in + m_width * m_height; ln != lnE; ln+= m_width, d+= padd ) {
                    int shift = 8;
                    *d = 0;
                    for(auto i = ln, lnE = ln + m_width; i != lnE; i++ ) {
                        shift -= m_bitDepth;

                        if constexpr(std::is_floating_point_v<value_type>) {
                            *d+= ( uint8_t(    float( *i ) * scaleNum / scaleDenom ) & mask ) << shift;
                        } else {
                            *d+= ( uint8_t( uint16_t( *i ) * scaleNum / scaleDenom ) & mask ) << shift;
                        }

                        if(shift == 0) {
                            shift = 8;
                            *(++d) = 0;
                        }
                    }
                }
            }
            int i = 0;
            m_rowPointers.resize(m_height);
            for( uint8_t* p = m_imageData.data(), *pE = p + lnSz * m_height; p != pE; p+= lnSz )
                m_rowPointers[i++] = p;
        }
        return true;
    }

    bool write(std::ostream& os) {
        if( os.bad() )
            return false;

             
        png_set_write_fn( m_pPngW, &os, &swtfn, nullptr );

        // Set PNG header info
        png_set_IHDR(m_pPngW, m_pInfo, m_width, m_height,
                     m_bitDepth, m_colorType, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        if(m_colorType == PNG_COLOR_TYPE_PALETTE)
            png_set_PLTE(m_pPngW, m_pInfo, m_paletteData.data(), int(m_paletteData.size()));

        png_write_info(m_pPngW, m_pInfo);

        png_write_image(m_pPngW, m_rowPointers.data());

        // Finish writing
        png_write_end(m_pPngW, nullptr);

        return true;
    }

    // write to file
    bool write(std::filesystem::path const& path) {
        return write((std::ostream&)std::ofstream(path, std::ios::binary | std::ios::out));
    }

    // write to memory
    bool write(char* out, size_t size) {
        return write((std::ostream&)omemstream(out, size ));
    }

    void convertRGB8toRGBA8( uint8_t* out ) const
    {
        uint8_t const* in = m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = *in++;
            *o++ = *in++;
            *o++ = *in++;
            *o++ = 255;
        }
    }

    void convertRGB8toRGB32F( float* out, int scaleNum = 1, int scaleDenom = 255 ) const
    {
        uint8_t const* in = m_imageData.data();
        auto o = out;
        float scale = float(scaleNum) / float(scaleDenom);
        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = float(*in++) * scale;
            *o++ = float(*in++) * scale;
            *o++ = float(*in++) * scale;
        }
    }

    void convertRGBA8toRGB32F( float* out, int scaleNum = 1, int scaleDenom = 255 ) const
    {
        uint8_t const* in = m_imageData.data();
        auto o = out;
        float scale = float(scaleNum) / float(scaleDenom);
        for( auto e = in + ptrdiff_t( m_width ) * m_height * 4; in != e; )
        {
            *o++ = float(*in++) * scale;
            *o++ = float(*in++) * scale;
            *o++ = float(*in++) * scale;
			in++; // skip alpha
        }
    }

    void convertRGB16toRGBA8( uint8_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = 255;
        }
    }

    void convertRGBA16toRGBA8( uint8_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * m_height * 3; in != e; )
        {
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
            *o++ = uint8_t( ( *in++ ) >> 8 );
        }
    }

    void convertRGB16toRGBA16( uint16_t* out ) const
    {
        uint16_t const* in = (uint16_t const*)m_imageData.data();
        auto o = out;

        for( auto e = in + ptrdiff_t( m_width ) * 3; in != e; )
        {
            *o++ = *in++;
            *o++ = *in++;
            *o++ = *in++;
            *o++ = 65535;
        }
    }

    bool convertGrayToGray8( uint8_t* out ) const
    {
        if(m_colorType != PNG_COLOR_TYPE_GRAY )
            return false;

        const size_t lnSz = size_t(int64_t(m_width) * m_bitDepth + 7) / 8;
        auto d = m_imageData.begin();
        auto dE = d + lnSz;
        const auto upShift = 8 - m_bitDepth;
        const auto mask = (1 << m_bitDepth) - 1;
        const auto padd = lnSz - size_t(m_width) * m_bitDepth / 8;
        for(auto ln = out, lnE = out + m_width * m_height; ln != lnE; ln+= m_width, d+= padd ) {
            int shift = 8;
            for(auto i = ln, lnE = ln + m_width; i != lnE; i++ ) {
                shift -= m_bitDepth;

                *i = (((*d) >> shift) & mask ) << upShift;
                   
                if(shift == 0) {
                    shift = 8;
                    d++;
                }
            }
        }
        return true;
    }

    bool convertPalettetoRGB8( uint8_t* out ) const
    {
        if(m_colorType != PNG_COLOR_TYPE_PALETTE)
            return false;

        const size_t lnSz = size_t(int64_t(m_width) * m_bitDepth + 7) / 8;
        auto d = m_imageData.begin();
        auto dE = d + lnSz;
        const auto padd = lnSz - size_t(m_width) * m_bitDepth / 8;
        const auto mask = (1 << m_bitDepth) - 1;
        for(auto ln = out, lnE = out + m_width * m_height * 3; ln != lnE; ln+= 3 * m_width, d+= padd ) {
            int shift = 8;
            for(auto i = ln, lnE = ln + m_width; i != lnE; i+= 3 ) {
                shift -= m_bitDepth;

                int pal = ((*d) >> shift) & mask;
                i[0] = m_paletteData[pal].red;
                i[1] = m_paletteData[pal].green;
                i[2] = m_paletteData[pal].blue;

                if(shift == 0) {
                    shift = 8;
                    d++;
                }
            }
        }

        return true;
    }

    bool convertPalettetoRGBA8( uint8_t* out ) const
    {
        if(m_colorType != PNG_COLOR_TYPE_PALETTE)
            return false;

        const size_t lnSz = size_t(int64_t(m_width) * m_bitDepth + 7) / 8;
        auto d = m_imageData.begin();
        auto dE = d + lnSz;
        const auto padd = lnSz - size_t(m_width) * m_bitDepth / 8;
        const auto mask = (1 << m_bitDepth) - 1;
        for(auto ln = out, lnE = out + m_width * m_height * 3; ln != lnE; ln+= 3 * m_width, d+= padd ) {
            int shift = 8;
            for(auto i = ln, lnE = ln + m_width; i != lnE; i+= 4 ) {
                shift -= m_bitDepth;

                int pal = ((*d) >> shift) & mask;
                i[0] = m_paletteData[pal].red;
                i[1] = m_paletteData[pal].green;
                i[2] = m_paletteData[pal].blue;
                i[3] = 255;

                if(shift == 0) {
                    shift = 8;
                    d++;
                }
            }
        }

        return true;
    }

};
