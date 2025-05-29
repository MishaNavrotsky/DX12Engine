// AssetsCreator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "GLTFStreamReader.h"
#include "AssetWriter.h"
#include "AssetReader.h"

int main()
{
    auto meshes = GLTFLocal::GetMeshesInfo("D:\\DX12En\\Engine\\assets\\glb\\alicev2rigged.glb", true);
    for (auto& mesh : meshes) {
        AssetsCreator::Asset::AssetWriter::Write(*mesh);
    }
    auto h = AssetsCreator::Asset::AssetReader::ReadMeshHeaders("D:\\DX12En\\AssetsCreator\\assets\\alicev2rigged_0.mesh.asset");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
