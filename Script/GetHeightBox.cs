using System;
using UnityEngine;

public class GetHeightBox : MonoBehaviour
{
    public string filePath = "Assets/HeightBox.txt";
    private Transform objectTransform;

    void Start()
    {
        objectTransform = GetComponent<Transform>();
    }

    void Update()
    {
        int num = objectTransform.childCount;
        string boundingBoxInfo = "";
        boundingBoxInfo += num;
        boundingBoxInfo += "\n";

        int cnt = 0;
        foreach (Transform childTransform in objectTransform)
        {
            Renderer childRenderer = childTransform.GetComponent<Renderer>();
            if (childRenderer != null)
            {
                Bounds bounds = childRenderer.bounds;

                Vector3 position = childTransform.position;
                Quaternion rotation = childTransform.rotation;
                Vector3 scale = childTransform.lossyScale;

                string positionString = VectorToString(position);
                string rotationString = QuaternionToString(rotation);
                string scaleString = VectorToString(scale);

                Vector3 center = bounds.center;
                Vector3 extent = bounds.extents;

                string centerString = VectorToString(center);
                string extentString = VectorToString(extent);

                int type = -1;
                if (childRenderer.CompareTag("Height0"))
                    type = 0;
                else if (childRenderer.CompareTag("Height1"))
                    type = 1;
                else if (childRenderer.CompareTag("Height2"))
                    type = 2;
                else if (childRenderer.CompareTag("Height3"))
                    type = 3;
                else if (childRenderer.CompareTag("Height4"))
                    type = 4;
                else if (childRenderer.CompareTag("Height5"))
                    type = 5;
                else if (childRenderer.CompareTag("Height6"))
                    type = 6;
                else if (childRenderer.CompareTag("Height7"))
                    type = 7;
                else if (childRenderer.CompareTag("Height8"))
                    type = 8;

                string childInfo = cnt + " " + type + " " + centerString + " " + extentString + " " + positionString + " " + rotationString + " " + scaleString + "\n";

                boundingBoxInfo += childInfo;
                cnt++;
            }
        }

        //총 오브젝트 개수
        //object번호 / 박스 중심 / 박스 크기 / 오브젝트 포지션 / 오브젝트 로테이션 / 오브젝트 스케일
        System.IO.File.WriteAllText(filePath, boundingBoxInfo);
    }

    private string VectorToString(Vector3 vector)
    {
        return vector.x + " " + vector.y + " " + vector.z;
    }

    private string QuaternionToString(Quaternion quaternion)
    {
        return quaternion.x + " " + quaternion.y + " " + quaternion.z + " " + quaternion.w;
    }
}
