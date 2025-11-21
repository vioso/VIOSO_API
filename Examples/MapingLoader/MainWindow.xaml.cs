// VIOSO API c# example using SharpDX
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

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
using SharpDX;
using SharpDX.Direct3D;
using SharpDX.Direct3D11;
using SharpDX.Mathematics;
using VIOSOWarpBlend;
using Matrix = SharpDX.Matrix;
namespace MapingLoader
{
    /// <summary>
    /// Interaktionslogik für MainWindo_warper.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        Warper _warper;
        public MainWindow()
        {
            InitializeComponent();
            try
            {
                string[] args = Environment.GetCommandLineArgs();
                string filepath;
                if (args.Length > 1)
                    filepath = args[1];
                else
                    filepath = "warp.vwf";
                // get info of addressed vwf
                Warper.WarpBlendHeader[] infos;
                VIOSOWarpBlend.Warper.GetVwfInfo(filepath, out infos); // actually it is allowed to specify multiple, comma separated files

                // create a warper for each entry
                for (int iInfo = 0; iInfo != infos.Length; iInfo++ )
                {
                    // we should, but must not, address an ini-file, as there might be global a parameters of interest in it
                    // set to String.Empty to go by fixed default values. Be aware, these might change in future, so update ALL values later
                    _warper = new Warper(Warper.DummyDevice, "VIOSOWarpBlend.ini", infos[iInfo].header.name );

                    // if you need to, you might adjust values read from ini
                    // these live in the dll's unmanaged memory, so we need to Get() and Set()
                    Warper.VWB_Warper ini = _warper.Get();

                    // change some ini value
                    ini.calibFile = infos[iInfo].path;
                    ini.calibIndex = iInfo;
                    ini.bAutoView = true;
                    ini.bFlipDXVs = true;

                    // update
                    _warper.Set(ref ini);
                    Warper.ERROR err = _warper.Init();
                    if (Warper.ERROR.NONE != err)
                        throw new ArgumentException("Could not initialize Warper. Err:" + err.ToString());
                    Warper.WarpFileHeader5 header;
                    _warper.GetWarpBlendHeader(out header);
                    String path;
                    _warper.GetMappingFilePath(out path);
                    IntPtr warpmap;
                    _warper.GetWarpMap(out warpmap);
                    bool b3D = 0 != (header.flags & (uint)Warper.FLAGS.IS3D);

                    Warper.Mesh m;
                    _warper.GetWarpBlendMesh(17, 17, out m);

                    // the raw data are a 2D texture RGBA32F, get size from header.width and .height
                    // warpmap must be present, no need to test
                    // to access each pixel's data use
                    if (IntPtr.Zero != warpmap)
                    {
                        for (Int32 i = 0; i != header.width * header.height; i++)
                        {
                            Warper.WARPRECORD wr = Marshal.PtrToStructure<Warper.WARPRECORD>(warpmap + i * Marshal.SizeOf(typeof(Warper.WARPRECORD)));
                            if (b3D)
                            {
                                if (0 < wr.w)
                                {
                                    Warper.VEC3 xyz = new Warper.VEC3(wr.x, wr.y, wr.z);
                                }
                            }
                            else
                            {
                                if (0 < wr.z)
                                {
                                    Warper.VEC2 uv = new Warper.VEC2(wr.x, wr.y);
                                }
                            }
                        }
                    }

                    // Unity hint
                    // Texture2D texWarp( RGBA32F, header.width, header.height );
                    // texWarp.LoadRawTextureData(rawData, header.width * header.height * Marshal.SizeOf(typeof(Warper.WARPRECORD)));

                    IntPtr blendmap;
                    _warper.GetBlendMap(out blendmap);
                    // the raw data are 2D texture, depending on header.flags:
                    //   if BLENDV3 & header.flags RGBA32F
                    //   if BLENDV2 & header.flags RGBA16U
                    //   else RGBA8U
                    if (IntPtr.Zero != blendmap)
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
                    _warper.GetBlackMap(out blackmap);
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
                    _warper.GetWhiteMap(out whitemap);
                    // the raw data are a 2D texture RGBA32F, get size from header.width and .height
                    // to access each pixel's data use
                    if (IntPtr.Zero != whitemap)
                    {
                        for (Int32 i = 0; i != header.width * header.height; i++)
                        {
                            Warper.BLENDRECORD3 br = Marshal.PtrToStructure<Warper.BLENDRECORD3>(whitemap + i * Marshal.SizeOf(typeof(Warper.BLENDRECORD)));
                        }
                    }

                    Warper.VEC3 eye = new Warper.VEC3(0, 0, 0);
                    Warper.VEC3 rot = new Warper.VEC3(0, 0, 0);
                    Warper.VEC3 pos = new Warper.VEC3();
                    Warper.VEC3 dir = new Warper.VEC3();
                    Warper.CLIP clip = new Warper.CLIP();
                    _warper.GetPosDirClip(ref eye, ref rot, ref pos, ref dir, ref clip);

                    var mP = Matrix.OrthoOffCenterRH(-clip.l, clip.r, -clip.b, clip.t, clip.n, clip.f);
                    var mR = Matrix.RotationYawPitchRoll(-dir.y, dir.x, dir.z);
                    var mT = Matrix.Translation(pos.x, pos.y, pos.z);
                    var mV = Matrix.Multiply( mR, mT );
                    var mVP = Matrix.Multiply( mV, mP ); 
                    var mVP_warper = new VIOSOWarpBlend.Warper.MAT4X4();
                    _warper.GetShaderVPMatrix(ref mVP_warper); // this is transposed
                }
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.ToString());
            }
        }
    }
}
