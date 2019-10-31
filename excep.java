import java.util.*;

class InvalidAgeException extends Exception 
{
	InvalidAgeException(String s) 
	{
		super(s);
	}
}

/*class Testhrow 
{
    public  static void validate (int age) throws InvalidAgeException 
    {
		if(age < 18) 
		{
			throw new InvalidAgeException("Not valid");
		}
		else 
		{
			System.out.println("Valid");
		}
	}
}*/
class excep 
{
	public static void main(String args[]) 
	{
		Scanner sc = new Scanner(System.in);
		int age = sc.nextInt();
		try 
		{
			if(age < 18) 
			{
				throw new InvalidAgeException("Not valid");
			}
			else 
			{
				System.out.println("Valid");
			}
		}
		catch (Exception e) 
		{
			System.out.println("Exception Occurs");
		}
		finally 
		{
			System.out.println("Rest of the code");
		}
	
	} 
}