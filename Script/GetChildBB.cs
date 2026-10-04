using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GetChildBB : MonoBehaviour
{
    public string filePath = "Assets/BoundingBox.txt";
    private Transform objectTransform;

    void Start()
    {
        objectTransform = GetComponent<Transform>();
    }

    // Update is called once per frame
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
                Vector3 scale = new Vector3(objectTransform.localScale.x * childTransform.localScale.x, objectTransform.localScale.y * childTransform.localScale.y, objectTransform.localScale.z * childTransform.localScale.z);

                string positionString = VectorToString(position);
                string rotationString = QuaternionToString(rotation);
                string scaleString = VectorToString(scale);

                Vector3 center = bounds.center;
                Vector3 extent = bounds.extents;

                string centerString = VectorToString(center);
                string extentString = VectorToString(extent);

                string childInfo = cnt + " " + centerString + " " + extentString + " " + positionString + " " + rotationString + " " + scaleString + "\n";

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