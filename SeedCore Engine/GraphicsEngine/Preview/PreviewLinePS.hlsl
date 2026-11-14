#include <GraphicsEngine/Preview/Preview.hlsli>

/**
* [EN]
* Preview.Draw.Line pixel shader: the line's color, alpha-blended over the
* mesh.
*
* ---------------------------------------------------------------------
*
* [JP]
* Preview.Draw.Line のピクセルシェーダー。線の色を、メッシュの上へアルファで重ねる。
*/
float4 main(PreviewLineOutput input) : SV_Target0
{
	return input.color;
}
