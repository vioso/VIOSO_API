// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

GLchar const* s_szPasstrough_vertex_shader_v110 = R"END(
#version 110
void main()
{
	gl_Position=gl_Vertex;			
	gl_TexCoord[0]=gl_MultiTexCoord0;
}
)END";

GLchar const* s_szPasstrough_vertex_shader_v330 = R"END(
#version 330
in vec2 TexCoord;
in vec3 Position;

out vec2 texcoord;

const vec2 pos[4] =
vec2[4](
	vec2(-1.0, 1.0),
	vec2(-1.0,-1.0),
	vec2( 1.0, 1.0),
	vec2( 1.0,-1.0)
);

const vec2 tex[4] =
vec2[4](
	vec2(0,0),
	vec2(0,1),
	vec2(1,0),
	vec2(1,1)
);

void main(void) {
	gl_Position = vec4( pos[gl_VertexID].x, pos[gl_VertexID].y, 0.0, 1.0);
	texcoord = tex[gl_VertexID];
}
)END";

// domeprojection param array:
// 0: gamma, // gamma linearization for mapping textures
// 1: warping, // 0: no warping, 1: warping
// 2: blending, // 0: no blending, 1: blending
// 3: bla, // 0: no black level adjustment, otherwise it is multiplied to the sampled value
// 4: secondary blending, // 0: no secondary blending, 1: secondary blending
// 5: input gamma, // input gamma for content
// 6: output gamma reciprocal, // 1 / output gamma for content
// 7: color correction, // 0: no color correction, otherwise it is multiplied to the sampled value
// 8: flip_v, // 0 : no flip, 1: flip vertically
// 10-15: reserved

GLchar const* s_sz_vertex_shader_v330_dp = R"END(
#version 330
layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texcoord;
layout(location = 3) in vec3 normal;
layout(location = 4) in vec3 tangent;

out vec2 uv_mapping;
out vec2 uv_content;
out vec2 uv_directional_shading;

uniform float params[16];
uniform mat4 matView;
uniform vec3 camPos;

void main(void) {
	vec4 pos = matView * vec4( position, 1 );
	// pass through texture coordinate, to sample from mappings
	uv_mapping = vec2( texcoord.x, params[8] > 0.0 ? 1.0 - texcoord.y : texcoord.y );

	// get content uv from normalized device coordinates
	uv_content = pos.xy;
	uv_content /= pos.w;
	uv_content.x += 1.0;
	uv_content.x *= 0.5;
	if( params[8] > 0.0 ) {
		uv_content.y += 1.0;
		uv_content.y *= 0.5;
	} else {
		uv_content.y -= 1.0;
		uv_content.y *= -0.5;
	}
		
	// calculate the color correction look up
	vec3 dir = camPos.xyz - position;
	// if direction is too flat, or no normals given, use a default direction
	if( params[7] > 0.0 && length(dir) > 0.0001 && ( normal.x != 0 || normal.y != 0 && normal.z != 0 ) )  {
		dir = normalize( dir );
		vec3 bitan = normalize( cross( normal, tangent ) );
		// projecting eye by tangent and bitangent effectively gives a perspective mapping
		vec2 dvec = vec2( dot( dir, tangent ), dot( dir, bitan ) );
		// go from perspective to spherical mapping
		dvec *= acos( clamp( dot( dir, normal ), -1.0, 1.0 ) ) / 1.57079632679489661923;
		uv_directional_shading = vec2( ( dvec.x + 1.0 ) / 2.0, ( dvec.y + 1.0 ) / 2.0 );
	} else {
		uv_directional_shading = vec2( 0.5, 0.5 ); // neutral direction
	}
}

)END";

GLchar const* s_fragment_shader_header_v110 = R"END(
#version 110				
uniform sampler2D samContent, samWarp, samBlend,samBlack;
uniform bool bBorder;
uniform bool bDoNotBlend;
uniform bool bDoNoBlack;		
uniform mat4 matView;		
uniform vec4 blackBias;
uniform vec4 offsScale;
#define texcoord gl_TexCoord[0]
#define FragColor gl_FragColor
vec4 _tex2D( sampler2D sam, vec2 tex ){ return texture2D( sam, tex ); }
)END";

GLchar const* s_fragment_shader_header_v330 = R"END(
#version 330
uniform sampler2D samContent, samWarp, samBlend, samBlack;
uniform bool bBorder;	
uniform bool bDoNotBlend;
uniform bool bDoNoBlack;
uniform mat4 matView;	
uniform vec4 blackBias;
uniform vec4 offsScale;
in vec2 texcoord;		
out vec4 FragColor;	
vec4 _tex2D( sampler2D sam, vec2 tex ){ return texture( sam, tex ); }
)END";

GLchar const* s_fragment_shader_header_v330_dp = R"END(
#version 330
uniform sampler2D samContent, samBlend, samBlack, samBlend2, samDirectionalShading;
uniform float params[16];

in vec2 uv_mapping;
in vec2 uv_content;
in vec2 uv_directional_shading;

out vec4 FragColor;

vec4 _tex2D( sampler2D sam, vec2 tex ){ return texture( sam, tex ); }
)END";

GLchar const* s_func_tex2D_BC = R"END(
uniform vec4 paramsBicubic;
vec4 _texture2D( sampler2D texCnt,
				   vec2 vPos)		
{									
	vPos*= params.xy;				
	vec2 t = floor( vPos - 0.5 ) + vec2(0.5,0.5); // the nearest pixel
	vec2 w0 = vec2(1,1);
	vec2 w1 = vPos - t;	
	vec2 w2 = w1 * w1;	
	vec2 w3 = w2 * w1;	

	w0 = w2 - 0.5 * (w3 + w1);		
	w1 = 1.5 * w3 - 2.5 * w2 + 1.0;	
	w3 = 0.5 * (w3 - w2);			
	w2 = 1.0 - w0 - w1 - w3;		

	vec2 s0 = w0 + w1;				
	vec2 s1 = w2 + w3;				
	vec2 f0 = w1 / s0;				
	vec2 f1 = w3 / s1;				

	vec2 t0 = t - 1.0 + f0;			
	vec2 t1 = t + 1.0 + f1;			
	t0*= params.zw;					
	t1*= params.zw;					

	return
		( _tex2D( texCnt, t0 ) * s0.x +
		  _tex2D( texCnt, vec2( t1.x, t0.y ) ) * s1.x ) * s0.y +
		( _tex2D( texCnt, vec2( t0.x, t1.y ) ) * s0.x +			
		  _tex2D( texCnt, t1 ) * s1.x ) * s1.y;					
}																
)END";

GLchar const* s_func_tex2D = R"END(
vec4 _texture2D( sampler2D texCnt,								
				   vec2 vPos)									
{																
	return _tex2D( texCnt, vPos );								
}																
)END";

GLchar const* s_bypass_fragment_shader = R"END(
void main()													
{																
	FragColor = _texture2D( samContent,texcoord.st );			
	FragColor.a = 1.0;											
}																
)END";

GLchar const* s_warp_blend_fragment_shader = R"END(
void main()
{																
	vec4 tex = _tex2D( samWarp,texcoord.st );					
	vec4 blend = _tex2D( samBlend, texcoord.st );				
	vec4 black = _tex2D( samBlack, texcoord.st ) * blackBias.x;	
	if( 0.1 < blend.a )											
	{															
		tex.y = 1.0 - tex.y;									
		if( bBorder )											
		{														
		    tex.x*= 1.02;										
		    tex.x-= 0.01;										
		    tex.y*= 1.02;										
		    tex.y-= 0.01;										
		}	
		FragColor = _texture2D( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );			
		if( !bDoNotBlend )					
			FragColor.rgb*= blend.rgb;		
	}	
	else
	{	
		FragColor = vec4( 0.0,0.0,0.0,1.0 );
	}
	if( !bDoNoBlack )               
	{
		// degamma
		FragColor = pow( FragColor, vec4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vec4 blackd = pow( black, vec4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );

		// offset color to get min average black
		FragColor += blackBias.y * blackd;

		// scale down to avoid clipping vOut
		FragColor *= vec4(1,1,1,1) - blackBias.z * blackd;

		// regamma
		FragColor = pow( FragColor, vec4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		

		// do lower clamp to stay above common black, upper is done anyway
		FragColor = max( FragColor, black );				
	}
	FragColor.a = 1.0;
}
)END";

GLchar const* s_warp_blend_fragment_shader_3D = R"END(
void main()
{
	vec4 tex = _tex2D( samWarp, texcoord.st );
	vec4 blend = _tex2D( samBlend, texcoord.st );
	vec4 black = _tex2D( samBlack, texcoord.st ) * blackBias.x;
	if( 0.01 < blend.a )
	{
		tex/= blend.a;
		tex.a = 1.0;
		tex = matView * tex;
		tex.xy/= tex.w;
		tex.x/=2.0;
		tex.y/=2.0;
		tex.xy+= 0.5;
//		// test mappings and border fit
//		if( 0.01 <= tex.x && tex.x <= 0.99 && 0.01 <= tex.y && tex.y <= 0.99 )                    
//			FragColor = vec4( tex.x, 1 - tex.y, 1, 1 );
//		else if( 0 <= tex.x && tex.x <= 1 && 0 <= tex.y && tex.y <= 1 )                    
//			FragColor = vec4( tex.x, 1 - tex.y, 0, 1 );
//		else
//			FragColor = vec4( 0, 0, 0, 1 );
		FragColor = _texture2D( samContent, ( tex.xy - offsScale.xy ) * offsScale.zw );
		if( !bDoNotBlend )
			FragColor.rgb*= blend.rgb;
	}
	else
	{
		FragColor = vec4( 0.0,0.0,0.0,1.0 );
	}
	if( !bDoNoBlack )
	{
		// degamma
		FragColor = pow( FragColor, vec4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );
		vec4 blackd = pow( black, vec4( blackBias.w, blackBias.w, blackBias.w, 1.0 ) );

		// offset color to get min average black
		FragColor += blackBias.y * blackd;					

		// scale down to avoid clipping vOut
		FragColor *= vec4(1,1,1,1) - blackBias.z * blackd;
	
		// regamma
		FragColor = pow( FragColor, vec4( 1.0/blackBias.w, 1.0/blackBias.w, 1.0/blackBias.w, 1.0 ) );		

		// do lower clamp to stay above common black, upper is done anyways
		FragColor = max( FragColor, black );				
	}

	FragColor.a = 1.0;
}
)END";

GLchar const* s_warp_blend_fragment_shader_dp = R"END(
void main(void) {
	vec3 gamma = vec3(params[0], params[0], params[0]);
	vec3 inputGamma = vec3(params[5], params[5], params[5]);
	vec3 outputGamma = vec3(params[6], params[6], params[6]);

	// sample content
	vec3 output;
	if( params[9] > 0.0 )
		output = _texture2D( samContent, mix(  uv_mapping, uv_content, params[1] ) ).rgb;
	else
		output = _texture2D( samContent, mix(  uv_mapping, uv_content, params[1] ) ).rgb;
	output = pow( output, inputGamma ); // linearize content

	// apply directional shading, TODO: move linearization to texture loader
	if( params[7] > 0.0 ) {
		vec3 clcrt = _tex2D( samDirectionalShading, uv_directional_shading ).rgb * params[7];
		clcrt = pow( clcrt, gamma );
		output *= clcrt;
	}

	// apply blending, TODO: move linearization to texture loader
	if( params[2] > 0.0 ) {
		vec3 blend1 = _tex2D( samBlend, uv_mapping ).rgb; // blend is linearized before loading tex to GPU
		output *= blend1;
	}

	// apply secondary blending, TODO: move linearization to texture loader
	if( params[4] > 0.0 ) {
		vec3 blend2 = _tex2D( samBlend2, uv_mapping ).rgb;
		blend2 = pow( blend2, gamma );  // linearize
		output *= blend2;
	}

	// apply black level uplift, TODO: move linearization to texture loader
	if( params[3] > 0.0 ) {
		vec3 bla = _tex2D( samBlack, uv_mapping ).rgb * params[3];
		bla = pow( bla, gamma ); // linearize
		output = output * ( 1.0 - bla ) + bla;
	}

	// re-gamma and output
	FragColor = vec4( pow( output, outputGamma ), 1.0 );
}
)END";

GLchar const* s_bypass_fragment_shader_dp = R"END(
void main()													
{																
	FragColor = _texture2D( samContent, uv_mapping );			
	FragColor.a = 1.0;	
}																
)END";

