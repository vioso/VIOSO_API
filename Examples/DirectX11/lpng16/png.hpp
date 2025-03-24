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
                uint32_t data;
                Bin() : data(0) {};
                Bin( png_color const& col ) : data( uint32_t( col.red ) | ( uint32_t( col.green ) << 8 ) | ( uint32_t( col.blue ) << 16 ) ) {}
                Bin( int r, int g, int b ) : data( clamp_value( 0, 255, r ) | (clamp_value( 0, 255, g ) << 8) | (clamp_value( 0, 255, b ) << 16 ) ) {}
                inline explicit Bin( uint32_t d ) : data(d) {}

                // the less operator automaically orders bins in map, so first is always no uplift, if this exists
                inline bool operator<(Bin const& other) const { return data < other.data; }
                struct Hasher {
                    inline std::size_t operator()(Bin const& b) const { return std::hash<uint32_t>{}(b.data); }
                };
                inline bool operator==(Bin const& other) const { return data == other.data; }
                inline bool operator!=(Bin const& other) const { return data != other.data; }
                inline operator png_color() const {
                    return png_color{ uint8_t(data), uint8_t(data >> 8), uint8_t(data >> 16) };
                }

                inline Bin& average( Bin const& rhs) { 
                    data =
                        (((data & 0xFF) + (rhs.data & 0xFF)) / 2) |
                        (((data & 0xFF00) + (rhs.data & 0xFF00)) / 2) |
                        (((data & 0xFF0000) + (rhs.data & 0xFF0000)) / 2);
                    return *this;
                }

                inline static Bin average(Bin const& lhs, Bin const& rhs) { 
                    return Bin(lhs).average(rhs);
                }

                inline uint32_t dist2(Bin const& other) const { // the squared distance, used to find close colors
                    int32_t r = int32_t(data & 0xFF) - int32_t(other.data & 0xFF); r*= r;
                    int32_t g = ( int32_t(data & 0xFF00) - int32_t(other.data & 0xFF00) ) >> 8; g*= g;
                    int32_t b = ( int32_t(data & 0xFF000) - int32_t(other.data & 0xFF000) ) >> 16; b*= b;
                    return r + g + b;
                }
            } Bin;

            std::unordered_map<Bin, std::pair<Bin, int>, typename Bin::Hasher> levels; // mapping a color to a target color and a palette index
            // fill map with data
            if constexpr( std::is_floating_point_v<value_type> ) {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels)
                    levels.emplace(Bin(int(pB[nR] * scaleNum / scaleDenom), int(pB[nG] * scaleNum / scaleDenom), int(pB[nB] * scaleNum / scaleDenom)), std::pair<Bin, int>{ -1, -1 });
            } else {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels)
                    levels.emplace(Bin(int(pB[nR]) * scaleNum / scaleDenom, int(pB[nG]) * scaleNum / scaleDenom, int(pB[nB]) * scaleNum / scaleDenom), std::pair<Bin, int>{ -1, -1 });
            }

            // merge levels, if more than 2^depth
            const size_t maxLevels = size_t(1) << m_bitDepth;
            size_t nLevels = levels.size();
            while(nLevels > maxLevels)
            {
                auto m1 = levels.end();
                auto m2 = levels.end();

                // find closest pair
                uint32_t sMin = UINT_MAX;

                // we skip first entry, as this is usually (0,0,0)
                for(auto a = ++levels.begin(); a != levels.end(); a++)
                {
                    if(Bin(-1) != a->second.first ) // already referenced
                        continue;
                    for(auto b = a; ++b != levels.end(); )
                    {
                        if(Bin(-1) != b->second.first )
                            continue;

                        auto s = a->first.dist2(b->first);
                        if(s < sMin)
                        {
                            m1 = a;
                            m2 = b;
                            sMin = s;
                        }
                    }
                }

                // merge pair, no need to check, as there is always a valid pair
                // tree iterators are no L-values, so we need to create a new Bin and delete old then insert new
                auto avg = Bin::average(m1->first, m2->first);
                auto iIt = levels.emplace( std::move( avg ), std::pair<Bin, int>{ -1, -1 }).first;
                nLevels--;

                if(m1->first != iIt->first ) // if m1 is not same as new, we set target to the merged color
                    m1->second.first = iIt->first; // set target color
                if( m2->first != iIt->first )
                    m2->second.first = iIt->first;
            }

            // create palette indices
            // iterating the entries and enumerate all, that hasn't been merged down, and save palette data
            int pal = 0;
            m_paletteData.resize( size_t(1) << m_bitDepth);
            for(auto& l : levels) {
                if(l.second.first == Bin(-1)) {
                    m_paletteData[pal] = l.first;
                    l.second.second = pal++;
                }
            }
            // set palette indices for merged colors
            for(auto& l : levels) {
                if(l.second.first != Bin(-1))
                    l.second.second = levels[l.second.first].second;
            }

            // fill palette info and raw data
            std::vector<uint8_t> rawIndex(m_width* m_height, 0);
            size_t i = 0;
            if constexpr( std::is_floating_point_v<value_type> ) {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels, i++ )
                {
					Bin cc(int(pB[nR] * scaleNum / scaleDenom), int(pB[nG] * scaleNum / scaleDenom), int(pB[nB] * scaleNum / scaleDenom));
					rawIndex[i] = levels[cc].second;
                }
            } else {
                for(TI pB = in, pBE = pB + nChannels * m_width * m_height; pB != pBE && pB != inEnd; pB+= nChannels, i++)
                {
                    Bin cc(int(pB[nR]) * scaleNum / scaleDenom, int(pB[nG]) * scaleNum / scaleDenom, int(pB[nB]) * scaleNum / scaleDenom);
                    rawIndex[i] = levels[cc].second;
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
            for(auto i = ln, iE = ln + m_width; i != iE; i++ ) {
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
            for(auto i = ln, iE = ln + m_width; i != iE; i+= 3 ) {
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
            for(auto i = ln, iE = ln + m_width; i != iE; i+= 4 ) {
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
