Module Module1

    Sub Main()
        Dim test As Object
        Dim arr(9) As Short

        test = CreateObject("OmegaOnline.Test")

        test.Array_Test_In(arr.GetLength(0), arr)

    End Sub

End Module
