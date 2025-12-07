package com.qti.acg.apps.controllers.finch;

import com.finchtechnologies.android.sdk.definition.Quaternion;
import com.finchtechnologies.android.sdk.definition.Vector3;

public final class FinchMath {
    private static final Quaternion identity = new Quaternion(0f, 0f, 0f, 1f);
    private static final Vector3 right = new Vector3(1, 0, 0);
    private static final Vector3 up = new Vector3(0, 1, 0);
    private static final float eps = 1e-6f;

    private FinchMath() {
    }

    public static boolean equal(float a, float b) {
        return Math.abs(a - b) < eps;
    }

    public static boolean isNan(Quaternion q) {
        return (Float.isNaN(q.x) || Float.isNaN(q.y) || Float.isNaN(q.z) || Float.isNaN(q.w));
    }

    public static float magnitudeSqr(Quaternion q) {
        return (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    }

    public static Quaternion normalized(Quaternion q) {
        float len = magnitudeSqr(q);

        if (equal(len, 1)) {
            return q;
        }

        if (!equal(len, 0)) {
            float sqrt = (float) Math.sqrt(len);
            return new Quaternion(q.x / sqrt, q.y / sqrt, q.z / sqrt, q.w / sqrt);
        }

        return new Quaternion(0, 0, 0, 1);
    }

    public static Quaternion conjugated(Quaternion q) {
        return new Quaternion(-q.x, -q.y, -q.z, q.w);
    }

    public static Quaternion mult(Quaternion q1, Quaternion q2) {
        float ww = (q1.z + q1.x) * (q2.x + q2.y);
        float yy = (q1.w - q1.y) * (q2.w + q2.z);
        float zz = (q1.w + q1.y) * (q2.w - q2.z);
        float xx = ww + yy + zz;
        float qq = 0.5f * (xx + (q1.z - q1.x) * (q2.x - q2.y));

        float w = qq - ww + (q1.z - q1.y) * (q2.y - q2.z);
        float x = qq - xx + (q1.x + q1.w) * (q2.x + q2.w);
        float y = qq - yy + (q1.w - q1.x) * (q2.y + q2.z);
        float z = qq - zz + (q1.z + q1.y) * (q2.w - q2.x);

        return new Quaternion(x, y, z, w);
    }

    public static Quaternion getCoordQuat(Vector3 x, Vector3 y, Vector3 z) {
        Quaternion q1 = fromToRotation(right, x);
        Quaternion q2 = fromToRotation(apply(q1, up), y);
        return mult(q2, q1);
    }

    public static Vector3 apply(Quaternion quat, Vector3 vec) {
        // Since vec.W == 0, we can optimize quat * vec * quat^-1 as follows:
        // vec + 2.0 * cross(quat.xyz, cross(quat.xyz, vec) + quat.w * vec)
        Vector3 xyz = new Vector3(quat.x, quat.y, quat.z);
        Vector3 temp;
        Vector3 temp2;
        temp = crossProduct(xyz, vec);
        temp2 = new Vector3(vec.x * quat.w, vec.y * quat.w, vec.z * quat.w);
        temp = new Vector3(temp.x + temp2.x, temp.y + temp2.y, temp.z + temp2.z);
        temp = crossProduct(xyz, temp);
        temp = new Vector3(temp.x * 2, temp.y * 2, temp.z * 2);
        return new Vector3(vec.x + temp.x, vec.y + temp.y, vec.z + temp.z);
    }

    public static Quaternion convertQuaternion(Quaternion cq, Quaternion q) {
        return mult(mult(conjugated(cq), q), cq);
    }

    public static Quaternion fromToRotation(Vector3 from, Vector3 to) {
        Vector3 fromT = normalized(from);
        Vector3 toT = normalized(to);

        Vector3 v = crossProduct(fromT, toT);
        float sinT = length(v);
        float cos = dotProduct(fromT, toT);

        if (Math.abs(sinT) > 0) {
            v.x /= sinT;
            v.y /= sinT;
            v.z /= sinT;
        } else {
            v = new Vector3(0, 0, 1);
        }

        float tmp = 0.5f - 0.5f * cos;
        if (tmp < 0)
            tmp = 0;

        float sinp = (float) Math.sqrt(tmp);
        float cosp = (float) Math.sqrt(0.5f + 0.5f * cos);
        return new Quaternion(v.x * sinp, v.y * sinp, v.z * sinp, cosp);
    }

    public static boolean isNan(Vector3 v) {
        return (Float.isNaN(v.x) || Float.isNaN(v.y) || Float.isNaN(v.z));
    }

    public static float length(Vector3 v) {
        return (float) Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    public static float dotProduct(Vector3 v1, Vector3 v2) {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    public static Vector3 crossProduct(Vector3 v1, Vector3 v2) {
        return new Vector3(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
    }

    public static Vector3 minus(Vector3 v) {
        return new Vector3(-v.x, -v.y, -v.z);
    }

    public static Vector3 plus(Vector3 a, Vector3 b) {
        return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
    }

    public static Vector3 normalized(Vector3 v) {
        float len = v.x * v.x + v.y * v.y + v.z * v.z;

        if (equal(len, 1)) {
            return v;
        }

        if (!equal(len, 0)) {
            float sqrt = (float) Math.sqrt(len);
            return new Vector3(v.x / sqrt, v.y / sqrt, v.z / sqrt);
        }

        return new Vector3(0, 0, 0);
    }

    public static class CoordinateSystem {
        private static final Quaternion centeringQuat = new Quaternion(0, 0, 0, 1);

        private boolean isRightHandedCS;
        private Quaternion coordsQuaternion;

        public CoordinateSystem(Vector3 newX, Vector3 newY, Vector3 newZ) {
            isRightHandedCS = isRightHandedCS(newX, newY, newZ);
            coordsQuaternion = getCoords(newX, newY, newZ, isRightHandedCS);
        }

        private static boolean isRightHandedCS(Vector3 newX, Vector3 newY, Vector3 newZ) {
            Vector3 newXYCrossProduct = crossProduct(newX, newY);
            return (dotProduct(newXYCrossProduct, newZ) > 0);
            //Cause i x j = k
        }

        private static Quaternion getCoords(Vector3 newX, Vector3 newY, Vector3 newZ, boolean isRightHandedCS) {
            if (isRightHandedCS) {
                return getCoordQuat(newX, newY, newZ);
            } else {
                Vector3 minusX = new Vector3(-newX.x, -newX.y, -newX.z);
                Vector3 minusY = new Vector3(-newY.x, -newY.y, -newY.z);
                Vector3 minusZ = new Vector3(-newZ.x, -newZ.y, -newZ.z);
                return getCoordQuat(minusX, minusY, minusZ);
            }
        }

        public Quaternion transform(Quaternion q) {
            return mult(centeringQuat, convertQuaternion(coordsQuaternion, q));
        }

        public Vector3 transform(Vector3 v) {
            Quaternion cq = conjugated(coordsQuaternion);
            Vector3 applyRes = apply(cq, v);

            if (isRightHandedCS) {
                return applyRes;
            } else {
                return new Vector3(-applyRes.x, -applyRes.y, -applyRes.z);
            }
        }

        public Quaternion inverseTransform(Quaternion q) {
            Quaternion inverseCoordsQuat = conjugated(coordsQuaternion);
            return convertQuaternion(inverseCoordsQuat, mult(conjugated(centeringQuat), q));
        }

        public Vector3 inverseTransform(Vector3 v) {
            Vector3 argV = (isRightHandedCS ? v : new Vector3(-v.x, -v.y, -v.z));
            return apply(coordsQuaternion, argV);
        }
    }

    public static class Transform {
        public Vector3 Position;
        public Quaternion Orientation;

        public Transform() {
            Position = new Vector3(0, 0, 0);
            Orientation = new Quaternion(0, 0, 0, 1);
        }
    }
}
