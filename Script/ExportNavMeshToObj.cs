using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.IO;
using System.Text;


public class NewBehaviourScript : MonoBehaviour
{
    public string exportFilePath = "Asset/NavMeshData.obj";

    public void ExportNavMeshData()
    {
        UnityEngine.AI.NavMeshTriangulation triangulation = UnityEngine.AI.NavMesh.CalculateTriangulation();

        StringBuilder objBuilder = new StringBuilder();

        // Export vertices
        foreach (Vector3 vertex in triangulation.vertices)
        {
            objBuilder.AppendFormat("v {0} {1} {2}\n", vertex.x, vertex.y, vertex.z);
        }

        // Export faces
        for (int i = 0; i < triangulation.indices.Length; i += 3)
        {
            int index1 = triangulation.indices[i] + 1;
            int index2 = triangulation.indices[i + 1] + 1;
            int index3 = triangulation.indices[i + 2] + 1;
            objBuilder.AppendFormat("f {0} {1} {2}\n", index1, index2, index3);
        }

        // Save the obj data to a file
        File.WriteAllText(exportFilePath, objBuilder.ToString());

        Debug.Log("NavMesh data exported to: " + exportFilePath);
    }

    void Start()
    {
        ExportNavMeshData();
    }

    void Update()
    {
        
    }
}
