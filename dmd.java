import java.util.*;

class area
{
	void display()
	{
		System.out.println("Area");
	}
}

class triangle extends area
{
	Scanner sc = new Scanner(System.in);
	int b,h;
	void display()
	{
		System.out.println("Enter base & height:");
		b = sc.nextInt();
		h = sc.nextInt();
		System.out.println("Area of triangle:"+(0.5*b*h));
	}
}

class circle extends area
{
	Scanner sc = new Scanner(System.in);
	int r;
	void display()
	{
		System.out.println("Enter radius:");
		r = sc.nextInt();
		System.out.println("Area of triangle:"+(3.14*r*r));
	}
}

class dmd
{
	public static void main(String[] args) 
	{
		int ch;
		area a = null;
		Scanner sc = new Scanner(System.in);
		System.out.println("1.Triangle\n2.Circle\nEnter Choice:");
		ch = sc.nextInt();
		switch(ch)
		{
			case 1:
				a = new triangle();
				break;
			case 2:
				a = new circle();
				break;
		}
		a.display();
	}
}