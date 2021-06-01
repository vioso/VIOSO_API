using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Diagnostics;
using System.Runtime.InteropServices;

using VIOSOWarpBlend;

namespace MapingLoader
{
    /// <summary>
    /// Interaktionslogik für MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            try
            {
                Warper w = new Warper(Warper.DummyDevice, "VIOSOWarpBlend.ini", "Display1");
                Warper.VWB_Warper ini = w.Get();
                ini.bFlipDXVs = true;
                w.Set(ini);
                Warper.ERROR err = w.Init();
                if (Warper.ERROR.NONE != err)
                    throw new ArgumentException("Could not initialize Warper. Err:" + err.ToString());
                Warper.WarpFileHeader4 header;
                w.GetWarpBlendHeader(out header);
                String path;
                w.GetMappingFilePath(out path);
                IntPtr warpmap;
                w.GetWarpMap(out warpmap);
                // the raw data are a 2D texture RGBA32F, get size from header.width and .height
                // warpmap must be present, no need to test
                // to access each pixel's data use
                if (IntPtr.Zero != warpmap)
                {
                    for (Int32 i = 0; i != header.width * header.height; i++)
                    {
                        Warper.WARPRECORD wr = Marshal.PtrToStructure<Warper.WARPRECORD>(warpmap + i * Marshal.SizeOf(typeof(Warper.WARPRECORD)));
                    }
                }

                IntPtr blendmap;
                w.GetBlendMap(out blendmap);
                // the raw data are 2D texture, depending on header.flags:
                //   if BLENDV3 & header.flags RGBA32F
                //   if BLENDV2 & header.flags RGBA16U
                //   else RGBA8U
                if( IntPtr.Zero != blendmap)
                {
                    if ((header.flags & (uint)Warper.FLAGS.BLENDV3) != 0)
                    {
                        for (Int32 i = 0; i != header.width * header.height; i++)
                        {
                            Warper.BLENDRECORD3 br = Marshal.PtrToStructure<Warper.BLENDRECORD3>(blendmap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD3)));
                        }
                    }
                    else if ((header.flags & (uint)Warper.FLAGS.BLENDV2) != 0)
                    {
                        for (Int32 i = 0; i != header.width * header.height; i++)
                        {
                            Warper.BLENDRECORD2 br = Marshal.PtrToStructure<Warper.BLENDRECORD2>(blendmap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD2)));
                        }
                    }
                    else
                    {
                        for (Int32 i = 0; i != header.width * header.height; i++)
                        {
                            Warper.BLENDRECORD br = Marshal.PtrToStructure<Warper.BLENDRECORD>(blendmap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD)));
                        }
                    }
                }

                IntPtr blackmap;
                w.GetBlackMap(out blackmap);
                // the raw data are a 2D texture RGBA8U, get size from header.width and .height
                // NOTE: the blend map is scaled
                // black = samBlack.sample( tex ) * header.blackScale;
                // out+= header.blackDark * black;
                // out*= float4(1, 1, 1, 1) - header.blackDark * header.blackBright * black; // scale down to avoid clipping } vOut
                // out = max(out, black); // do lower clamp to stay above common black, upper is done anyways
                // to access each pixel's data use
                if (IntPtr.Zero != blackmap && 0 != header.blackScale)
                {
                    for (Int32 i = 0; i != header.width * header.height; i++)
                    {
                        Warper.BLENDRECORD br = Marshal.PtrToStructure<Warper.BLENDRECORD>(blackmap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD)));
                    }
                }

                IntPtr whitemap;
                w.GetWhiteMap(out whitemap);
                // the raw data are a 2D texture RGBA32F, get size from header.width and .height
                // to access each pixel's data use
                if( IntPtr.Zero != whitemap )
                {
                    for (Int32 i = 0; i != header.width * header.height; i++)
                    {
                        Warper.BLENDRECORD3 br = Marshal.PtrToStructure<Warper.BLENDRECORD3>(whitemap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD)));
                    }
                }

            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }
    }
}

